gtest_dep = dependency('gtest', main: true, disabler: true, required: false)
if not gtest_dep.found()
	cmake = import('cmake')
	gtest_subproject = cmake.subproject('gtest')
	cm_gtest_dep = gtest_subproject.dependency('gtest')
	cm_gtest_main_dep = gtest_subproject.dependency('gtest_main')
	gtest_dep = declare_dependency(dependencies: [ cm_gtest_dep,
						       cm_gtest_main_dep,
						       dependency('threads') ])
endif

test('test-inventory',
     executable('test-inventory',
	        sources: [ 'test-inventory.cpp',
			   'mock-inventory.cpp' ],
		dependencies: [ inventory_dep, inventory_test_dep, gtest_dep ]))

test('test-inventory-migrations',
     executable('test-inventory-migrations',
	        sources: [ 'test-inventory-migrations.cpp',
			   'mock-inventory.cpp' ],
	        dependencies: [ inventory_dep, inventory_test_dep, gtest_dep ]))

test('test-nvme',
     executable('test-nvme', 'test-nvme.cpp',
	        dependencies: [ headers_dep,
			        devices_dep,
				sysfs_dep,
				libgpiodcxx_dep,
				phosphor_logging_dep,
				gtest_dep ]))

test('test-platform',
     executable('test-platform',
	        sources: [ 'test-platform.cpp',
			   '../notify.cpp' ],
	        dependencies: [ headers_dep,
			        libgpiodcxx_dep,
			        phosphor_logging_dep,
			        gtest_dep ]))

test('test-lights-out',
     executable('test-lights-out',
	        sources: ['test-lights-out.cpp',
			  'mock-inventory.cpp',
			  '../notify.cpp' ],
	        dependencies: [ headers_dep,
			        devices_dep,
				inventory_dep,
				libgpiodcxx_dep,
				phosphor_logging_dep,
				gtest_dep ]))
