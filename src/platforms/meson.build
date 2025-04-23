subdir('bonnell')
subdir('everest')
subdir('rainier')

libgpiodcxx_dep = dependency(
    'libgpiodcxx',
    fallback: ['libgpiod'],
    required: true,
    default_options: [
        'buildtype=minsize',
        'default_library=static',
        'bindings=cxx',
        'strip=true',
    ],
)

platforms_dep = declare_dependency(
    dependencies: [bonnell_dep, everest_dep, rainier_dep, libgpiodcxx_dep],
)
