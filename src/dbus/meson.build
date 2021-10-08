sdbusplus_dep = dependency('sdbusplus',
			   fallback: [ 'sdbusplus', 'sdbusplus_dep' ])

dbus_dep = declare_dependency(sources: [ 'inventory.cpp' ],
			       dependencies: sdbusplus_dep)
