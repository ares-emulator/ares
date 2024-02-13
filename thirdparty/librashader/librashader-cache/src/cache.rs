use crate::cacheable::Cacheable;
use crate::key::CacheKey;

pub(crate) mod internal {
    use platform_dirs::AppDirs;
    use rusqlite::{Connection, DatabaseName};
    use std::error::Error;
    use std::path::PathBuf;

    pub(crate) fn get_cache_dir() -> Result<PathBuf, Box<dyn Error>> {
        let cache_dir = if let Some(cache_dir) =
            AppDirs::new(Some("librashader"), false).map(|a| a.cache_dir)
        {
            cache_dir
        } else {
            let mut current_dir = std::env::current_dir()?;
            current_dir.push("librashader");
            current_dir
        };

        std::fs::create_dir_all(&cache_dir)?;

        Ok(cache_dir)
    }

    pub(crate) fn get_cache() -> Result<Connection, Box<dyn Error>> {
        let cache_dir = get_cache_dir()?;
        let mut conn = Connection::open(&cache_dir.join("librashader.db"))?;

        let tx = conn.transaction()?;
        tx.pragma_update(Some(DatabaseName::Main), "journal_mode", "wal2")?;
        tx.execute(
            r#"create table if not exists cache (
        type text not null,
        id blob not null,
        value blob not null unique,
        primary key (id, type)
    )"#,
            [],
        )?;
        tx.commit()?;
        Ok(conn)
    }

    pub(crate) fn get_blob(
        conn: &Connection,
        index: &str,
        key: &[u8],
    ) -> Result<Vec<u8>, Box<dyn Error>> {
        let value = conn.query_row(
            &*format!("select value from cache where (type = (?1) and id = (?2))"),
            rusqlite::params![index, key],
            |row| row.get(0),
        )?;
        Ok(value)
    }

    pub(crate) fn set_blob(conn: &Connection, index: &str, key: &[u8], value: &[u8]) {
        match conn.execute(
            &*format!("insert or replace into cache (type, id, value) values (?1, ?2, ?3)"),
            rusqlite::params![index, key, value],
        ) {
            Ok(_) => return,
            Err(e) => println!("err: {:?}", e),
        }
    }
}

/// Cache a shader object (usually bytecode) created by the keyed objects.
///
/// - `factory` is the function that compiles the values passed as keys to a shader object.
/// - `load` tries to load a compiled shader object to a driver-specialized result.
pub fn cache_shader_object<E, T, R, H, const KEY_SIZE: usize>(
    index: &str,
    keys: &[H; KEY_SIZE],
    factory: impl FnOnce(&[H; KEY_SIZE]) -> Result<T, E>,
    load: impl Fn(T) -> Result<R, E>,
    bypass_cache: bool,
) -> Result<R, E>
where
    H: CacheKey,
    T: Cacheable,
{
    if bypass_cache {
        return Ok(load(factory(keys)?)?);
    }

    let cache = internal::get_cache();

    let Ok(cache) = cache else {
        return Ok(load(factory(keys)?)?);
    };

    let hashkey = {
        let mut hasher = blake3::Hasher::new();
        for subkeys in keys {
            hasher.update(subkeys.hash_bytes());
        }
        let hash = hasher.finalize();
        hash
    };

    'attempt: {
        if let Ok(blob) = internal::get_blob(&cache, index, hashkey.as_bytes()) {
            let cached = T::from_bytes(&blob).map(&load);

            match cached {
                None => break 'attempt,
                Some(Err(_)) => break 'attempt,
                Some(Ok(res)) => return Ok(res),
            }
        }
    };

    let blob = factory(keys)?;

    if let Some(slice) = T::to_bytes(&blob) {
        internal::set_blob(&cache, index, hashkey.as_bytes(), &slice);
    }
    Ok(load(blob)?)
}

/// Cache a pipeline state object.
///
/// Keys are not used to create the object and are only used to uniquely identify the pipeline state.
///
/// - `restore_pipeline` tries to restore the pipeline with either a cached binary pipeline state
///    cache, or create a new pipeline if no cached value is available.
/// - `fetch_pipeline_state` fetches the new pipeline state cache after the pipeline was created.
pub fn cache_pipeline<E, T, R, const KEY_SIZE: usize>(
    index: &str,
    keys: &[&dyn CacheKey; KEY_SIZE],
    restore_pipeline: impl Fn(Option<Vec<u8>>) -> Result<R, E>,
    fetch_pipeline_state: impl FnOnce(&R) -> Result<T, E>,
    bypass_cache: bool,
) -> Result<R, E>
where
    T: Cacheable,
{
    if bypass_cache {
        return Ok(restore_pipeline(None)?);
    }

    let cache = internal::get_cache();

    let Ok(cache) = cache else {
        return Ok(restore_pipeline(None)?);
    };

    let hashkey = {
        let mut hasher = blake3::Hasher::new();
        for subkeys in keys {
            hasher.update(subkeys.hash_bytes());
        }
        let hash = hasher.finalize();
        hash
    };

    let pipeline = 'attempt: {
        if let Ok(blob) = internal::get_blob(&cache, index, hashkey.as_bytes()) {
            let cached = restore_pipeline(Some(blob));
            match cached {
                Ok(res) => {
                    break 'attempt res;
                }
                _ => (),
            }
        }

        restore_pipeline(None)?
    };

    // update the pso every time just in case.
    if let Ok(state) = fetch_pipeline_state(&pipeline) {
        if let Some(slice) = T::to_bytes(&state) {
            internal::set_blob(&cache, index, hashkey.as_bytes(), &slice);
        }
    }

    Ok(pipeline)
}
