find_package(X11)

option(USE_QT6 "Use Qt6 UI backend" OFF)
mark_as_advanced(USE_QT6)

if(NOT USE_QT6)
  find_package(GTK REQUIRED)

  target_link_libraries(hiro PRIVATE GTK::GTK X11::X11)

  target_enable_feature(hiro "GTK3 UI backend")
  target_compile_definitions(hiro PUBLIC HIRO_GTK=3)
  set(HIRO_BACKEND "GTK3" PARENT_SCOPE)
else()
  find_package(Qt6 COMPONENTS Widgets REQUIRED)

  get_target_property(QT_MOC_EXECUTABLE Qt6::moc LOCATION)
  if(NOT QT_MOC_EXECUTABLE)
    message(FATAL_ERROR "Qt6 moc is not found: QT_MOC_EXECUTABLE is not set.")
  endif()

  execute_process(
    COMMAND ${QT_MOC_EXECUTABLE} -i -o ${CMAKE_CURRENT_SOURCE_DIR}/qt/qt.moc ${CMAKE_CURRENT_SOURCE_DIR}/qt/qt.hpp
  )

  target_link_libraries(hiro PRIVATE X11::X11 Qt6::Core Qt6::Gui Qt6::Widgets)

  target_enable_feature(hiro "Qt6 UI backend")
  target_compile_definitions(hiro PUBLIC HIRO_QT=6)
  set(HIRO_BACKEND "Qt" PARENT_SCOPE)
endif()

get_target_property(hiro_SOURCES hiro SOURCES)

set_source_files_properties(hiro ${hiro_SOURCES} PROPERTIES HEADER_FILE_ONLY TRUE)

set_source_files_properties(hiro hiro.cpp PROPERTIES HEADER_FILE_ONLY FALSE)
