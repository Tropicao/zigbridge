project('ZLL Gateway' , 'c')

src = ['src/main.c']
incdir = include_directories('/usr/local/include/libznp')

cc = meson.get_compiler('c')
cflags=['-Wall', '-Wextra', '-Werror']
dep = cc.find_library('znp', dirs: ['/usr/local/lib', '/usr/lib'])

executable('zll_gateway',
    sources: src,
    c_args: cflags,
    include_directories: incdir,
    dependencies: dep)
