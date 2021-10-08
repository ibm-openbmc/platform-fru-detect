subdir('sysfs')
subdir('platforms')
subdir('dbus')

fru_deps = [ sysfs_dep, platforms_dep, dbus_dep ]
phosphor_logging_dep = dependency('phosphor-logging',
				  fallback: [ 'phosphor-logging', 'phosphor_logging_dep' ])

executable('platform-fru-detect', 'platform-fru-detect.cpp', 'platform.cpp',
	   dependencies: [ fru_deps, sdbusplus_dep, phosphor_logging_dep ],
	   install: true)
