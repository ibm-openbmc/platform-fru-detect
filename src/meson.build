subdir('sysfs')
subdir('platforms')
subdir('dbus')

nvme_drive_deps = [ sysfs_dep, platforms_dep, dbus_deps ]
phosphor_logging_dep = dependency('phosphor-logging',
				  fallback: [ 'phosphor-logging', 'phosphor_logging_dep' ])

executable('platform-fru-detect', 'platform-fru-detect.cpp',
	   dependencies: [ sdbusplus_dep, nvme_drive_deps, phosphor_logging_dep ],
	   install: true)
