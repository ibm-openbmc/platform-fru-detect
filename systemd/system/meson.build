systemd_system_unit_dir = systemd.get_pkgconfig_variable('systemdsystemunitdir')

configure_file(input : 'platform-fru-detect.service',
               output: 'platform-fru-detect.service',
               copy: true,
               install_dir: systemd_system_unit_dir,
               install : true)
