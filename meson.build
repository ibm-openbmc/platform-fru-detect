project(
    'platform-fru-detect',
    'cpp',
    default_options: [
        'buildtype=minsize',
        'warning_level=3',
        'werror=true',
        'cpp_std=c++20',
        'b_lto=true',
        'b_ndebug=true',
    ],
    meson_version: '>=0.57',
    version: 'v0.1.0',
)

add_project_arguments(
    '-Wunused-function',
    '-Wunused-label',
    '-Wunused-value',
    '-Wunused-variable',
    language: 'cpp',
)

subdir('src')
subdir('systemd')
