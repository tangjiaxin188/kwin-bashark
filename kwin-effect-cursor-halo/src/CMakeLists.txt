kcoreaddons_add_plugin(cursorhalo INSTALL_NAMESPACE "kwin/effects/plugins/")

target_sources(cursorhalo PRIVATE
    cursorhalo.cpp
    main.cpp
)

target_link_libraries(cursorhalo
    KF6::CoreAddons
    KWin::kwin
)
