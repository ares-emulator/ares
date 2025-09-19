# dummy GitHub target used to enable editing of GitHub files within generated IDE projects

add_library(.github INTERFACE)

set(.github_sources
    ISSUE_TEMPLATE/bug_report.md
    ISSUE_TEMPLATE/feature_request.md
    ISSUE_TEMPLATE/game-issue-report.md
    scripts/build_macos.sh
    scripts/build_ubuntu.sh
    scripts/build_windows.sh
    scripts/package_artifacts.sh
    workflows/build-aux.yml
    workflows/build.yml
    workflows/pr.yml
    workflows/push.yml
    workflows/release.yml
)

target_sources(
  .github
  PRIVATE
  ${.github_sources}
  ../.gitignore
  ../.gitattributes
  ../README.md
  ../LICENSE
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${.github_sources})

