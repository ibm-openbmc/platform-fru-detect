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
     executable('test-inventory', 'test-inventory.cpp',
		dependencies: [ inventory_dep, inventory_test_dep, gtest_dep ]))
