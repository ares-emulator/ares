# dummy GitHub target used to enable editing of GitHub files within generated IDE projects

add_library(.github INTERFACE)

set(.github_sources
    ISSUE_TEMPLATE/bug_report.md
    ISSUE_TEMPLATE/feature_request.md
    ISSUE_TEMPLATE/game-issue-report.md
    scripts/build_sdl.sh
    workflows/electron-macos.yml
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

