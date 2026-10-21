import { app } from 'electron'
const { sessions } = await import('../dist/main.js')
process.on('SIGTERM', () => app.quit())
app.whenReady().then(async () => {
  for (let attempt = 0; attempt < 300; attempt++) {
    try {
      if ((await sessions.request('session.status')).result) {
        console.log('PROFILE_READY')
        return
      }
    } catch {}
    await new Promise(resolve => setTimeout(resolve, 20))
  }
  console.error('profile startup timeout')
  app.exit(1)
})
