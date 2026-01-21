cmake_minimum_required(VERSION 3.22)

project(MusicMaker VERSION 0.1.0)

# Set the path to JUCE
set(JUCE_PATH "C:/JUCE" CACHE PATH "Path to JUCE")
add_subdirectory(${JUCE_PATH} _juce)

# Manually set WebView2 paths to bypass FindWebView2.cmake logic if it fails
set(WebView2_include_dir "C:/webview2/Microsoft.Web.WebView2.1.0.2903.40/build/native/include" CACHE PATH "")
set(WebView2_library "C:/webview2/Microsoft.Web.WebView2.1.0.2903.40/build/native/x64/WebView2LoaderStatic.lib" CACHE PATH "")
set(WebView2_FOUND TRUE CACHE BOOL "")

juce_add_gui_app(MusicMaker
    PRODUCT_NAME "Music Maker"
    COMPANY_NAME "GeminiCLI"
    BUNDLE_ID com.geminicli.musicmaker
    IS_CONTENT_BROWSER TRUE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
)

juce_generate_juce_header(MusicMaker)

target_sources(MusicMaker
    PRIVATE
        Source/Main.cpp
        Source/MainComponent.h
        Source/MainComponent.cpp
)

target_compile_definitions(MusicMaker
    PRIVATE
        JUCE_USE_WIN_WEBVIEW2=1
)

target_link_libraries(MusicMaker
    PRIVATE
        juce::juce_audio_utils
        juce::juce_gui_extra
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Bundle Resources
juce_add_binary_data(BinaryData
    SOURCES
        Resources/index.html
)

target_link_libraries(MusicMaker PRIVATE BinaryData)
