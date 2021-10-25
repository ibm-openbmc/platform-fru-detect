subdir('sysfs')
subdir('platforms')

sdbusplus_dep = dependency('sdbusplus', fallback: [ 'sdbusplus', 'sdbusplus_dep' ])
phosphor_logging_dep = dependency('phosphor-logging',
				  fallback: [ 'phosphor-logging', 'phosphor_logging_dep' ])

inventory_dep = declare_dependency(sources: 'inventory.cpp',
				   include_directories: '.',
				   dependencies: [ sdbusplus_dep, phosphor_logging_dep ])

fru_deps = [ sysfs_dep, platforms_dep, inventory_dep ]

executable('platform-fru-detect',
	   'platform-fru-detect.cpp', 'platform.cpp', 'inventory.cpp', 'notify.cpp',
	   dependencies: [ fru_deps, phosphor_logging_dep ],
	   install: true)

if not get_option('tests').disabled()
    subdir('tests')
endif
