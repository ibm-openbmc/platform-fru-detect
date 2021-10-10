subdir('sysfs')
subdir('platforms')
subdir('devices')

fru_deps = [ sysfs_dep, platforms_dep, devices_dep ]
sdbusplus_dep = dependency('sdbusplus', fallback: [ 'sdbusplus', 'sdbusplus_dep' ])
phosphor_logging_dep = dependency('phosphor-logging',
				  fallback: [ 'phosphor-logging', 'phosphor_logging_dep' ])

executable('platform-fru-detect', 'platform-fru-detect.cpp', 'platform.cpp', 'inventory.cpp',
	   dependencies: [ fru_deps, sdbusplus_dep, phosphor_logging_dep ],
	   install: true)
