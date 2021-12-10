subdir('sysfs')
subdir('platforms')
subdir('devices')
subdir('inventory')

sdbusplus_dep = dependency('sdbusplus', fallback: [ 'sdbusplus', 'sdbusplus_dep' ])
phosphor_logging_dep = dependency('phosphor-logging',
				  fallback: [ 'phosphor-logging', 'phosphor_logging_dep' ])

# libi2c doesn't ship a pkgconfig file!
cpp = meson.get_compiler('cpp')
i2c_dep = cpp.find_library('i2c')

# Used by inventory tests
inventory_test_dep = declare_dependency(sources: [ 'dbus.cpp' ],
				   include_directories: '.',
				   dependencies: [ inventory_dep,
						   sdbusplus_dep,
						   phosphor_logging_dep ])

fru_deps = [ sysfs_dep, platforms_dep, inventory_dep, devices_dep ]

platform_fru_detect_sources = [
    'dbus.cpp',
    'environment.cpp',
    'i2c.cpp',
    'notify.cpp',
    'platform.cpp',
    'platform-fru-detect.cpp',
]

executable('platform-fru-detect',
	   sources: platform_fru_detect_sources,
	   dependencies: [ fru_deps, phosphor_logging_dep, i2c_dep ],
	   install: true)

if not get_option('tests').disabled()
    subdir('tests')
endif
