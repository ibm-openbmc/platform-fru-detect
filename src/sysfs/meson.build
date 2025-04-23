subdir('i2c')

sysfs_dep = declare_dependency(
    sources: ['eeprom.cpp', 'devicetree.cpp'],
    dependencies: [sysfs_i2c_dep],
)
