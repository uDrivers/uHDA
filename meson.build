project('uhda', 'cpp',
	default_options : ['cpp_std=c++20']
)

sources = files(
	'src/uhda.cpp',
	'src/controller.cpp',
	'src/codec.cpp',
	'src/stream.cpp',
	'src/simple.cpp',
)

includes = include_directories('include')

if get_option('build_library')
	pkg = import('pkgconfig')

	lib = static_library('uhda', sources,
		include_directories : includes,
		install : true
	)

	install_headers(
		'include/uhda/kernel_api.h',
		'include/uhda/types.h',
		'include/uhda/uhda.h',
		subdir : 'uhda'
	)

	pkg.generate(lib)
endif
