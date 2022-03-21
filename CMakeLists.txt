cmake_minimum_required(VERSION 3.6.3)

set(gstexample "gstexample")
set(gst_scan_barcode "gst_scan_barcode")
set(videotestsrc_example "videotestsrc_example")
set(scan_image "scan_image")

find_package(PkgConfig REQUIRED)

pkg_check_modules(GST gstreamer-1.0)
if(GST_FOUND)
  link_directories(${GST_LIBRARY_DIRS}) 

  pkg_check_modules(GLIB2 REQUIRED glib-2.0)
  if(GLIB2_FOUND)
    link_directories(${GLIB2_LIBRARY_DIRS})
  else()
    message(FATAL_ERROR "glib-2.0 not found. Will not build gstexample")
  endif()
  pkg_check_modules(GST_APP REQUIRED gstreamer-app-1.0)
  if(GST_APP_FOUND)
    link_directories(${GST_APP_LIBRARY_DIRS})
  else()
    message(FATAL_ERROR "gstreamer-app-1.0 not found. Will not build gstexample")
  endif()
  pkg_check_modules(GOBJ2 REQUIRED gobject-2.0)
  if(GOBJ2_FOUND)
    link_directories(${GOBJ2_LIBRARY_DIRS})
  else()
    message(FATAL_ERROR "gobject-2.0 not found. Will not build gstexample")
  endif()

else()
  message(FATAL_ERROR "gstreamer not found. Will not build gstexample")
endif()

add_executable(${gstexample} gstexample.c)
add_executable(${videotestsrc_example} videotestsrc_example.c)
add_executable(${scan_image} scan_image.c)
add_executable(${gst_scan_barcode} gst_scan_barcode.c)


target_include_directories(${scan_image} PRIVATE incl)

target_include_directories(${gst_scan_barcode} PRIVATE ${GLIB2_INCLUDE_DIRS} ${GST_INCLUDE_DIRS} ${GST_APP_INCLUDE_DIRS} ${GOBJ2_INCLUDE_DIRS} incl)

target_include_directories(${gstexample} PRIVATE ${GLIB2_INCLUDE_DIRS} ${GST_INCLUDE_DIRS} ${GST_APP_INCLUDE_DIRS} ${GOBJ2_INCLUDE_DIRS})
target_include_directories(${videotestsrc_example} PRIVATE ${GLIB2_INCLUDE_DIRS} ${GST_INCLUDE_DIRS} ${GST_APP_INCLUDE_DIRS} ${GOBJ2_INCLUDE_DIRS})

target_link_libraries(${gstexample} pthread gstreamer-1.0 gstapp-1.0 gobject-2.0 glib-2.0)
target_link_libraries(${videotestsrc_example} pthread gstreamer-1.0 gstapp-1.0 gobject-2.0 glib-2.0)

target_link_libraries(${scan_image} pthread png png16 ${CMAKE_CURRENT_SOURCE_DIR}/libzbar.so)

target_link_libraries(${gst_scan_barcode} pthread gstreamer-1.0 gstapp-1.0 gobject-2.0 glib-2.0 png png16 ${CMAKE_CURRENT_SOURCE_DIR}/libzbar.so)
