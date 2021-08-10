subdir('sysfs')
subdir('platforms')
subdir('dbus')

nvme_drive_deps = [ sysfs_dep, platforms_dep, dbus_deps ]

executable('platform-fru-detect', 'platform-fru-detect.cpp',
	   dependencies: [ sdbusplus_dep, nvme_drive_deps ],
	   install: true)
