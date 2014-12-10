
import urllib2
import hashlib
import zipfile
from waflib.Task import Task
from waflib.Build import BuildContext

class DownloadSource(Task):
    def run(self):
        src_http = urllib2.urlopen(self.url)
        src_archive = src_http.read()
        src_http.close()

        src_hasher = hashlib.sha256()
        src_hasher.update(src_archive)

        if self.hash_sha256 != src_hasher.hexdigest():
            raise Exception('hash mismatch: expected ' + self.hash_sha256 + ' got ' + src_hasher.hexdigest())

        self.outputs[0].write(src_archive, flags='wb')

class UnzipArchive(Task):
    def run(self):
        arc_zip = zipfile.ZipFile(self.inputs[0].abspath())
        arc_zip.extractall(self.outputs[0].abspath())

def get_deps(ctx):
    deps = [
        {
            'name': 'glm',
            'url': 'http://softlayer-dal.dl.sourceforge.net/project/ogl-math/glm-0.9.5.4/glm-0.9.5.4.zip',
            'hash': 'c25002f109104bb8eb37a7e74c745cbc0a713ec5d9a857050c7878edb5ee246c'
        },
        {
            'name': 'tetgen',
            'url': 'http://wias-berlin.de/software/tetgen/1.5/src/tetgen1.5.0.zip',
            'hash': 'b1bb4563703dca277e75eecf40803ce62f9d75efc65918327708702269814eb7'
        },
        {
            'name': 'freetype',
            'url': 'http://download.savannah.gnu.org/releases/freetype/ft252.zip',
            'hash': 'c68b1972788ddb8a84cc29a16fda0f85b9948a93db739a147b9ae98e671f39a1'
        },
        {
            'name': 'glew',
            'url': 'https://sourceforge.net/projects/glew/files/glew/1.10.0/glew-1.10.0.zip/download',
            'hash': '43c6229d787673ac1d35ebaad52dfdcc78c8b55d13ee78d8e4d7e4a6cb72b050'
        },
        {
            'name': 'sfml',
            'url': 'http://www.sfml-dev.org/download/sfml/2.1/SFML-2.1-sources.zip',
            'hash': '5f46d7748223be3f0c6a9fcf18c0016d227f7b1903cdbcd85f61ddbc82ef95bf',
        },
        {
            'name': 'tgui',
            'url': 'https://github.com/texus/TGUI/archive/v0.6.6.zip',
            'hash': '0af220ca9f9dc279b8551fa682a3beaa512f9e776210443ebdfefad532e7a907',
        }
    ]

    src_nodes = {}

    deps_node = ctx.path.make_node('deps')
    deps_node.mkdir()
    for dep in deps:
        dep_name = dep['name']
        archive_node = deps_node.make_node(dep_name + '_download')
        src_node = deps_node.make_node(dep_name + '_src')

        # Downloading Task
        dl_task = DownloadSource(env=ctx.env)
        dl_task.url = dep['url']
        dl_task.hash_sha256 = dep['hash']
        dl_task.set_outputs(archive_node)
        ctx.add_to_group(dl_task)

        # Extraction Task
        unzip_task = UnzipArchive(env=ctx.env)
        unzip_task.set_inputs(archive_node)
        unzip_task.set_outputs(src_node)
        ctx.add_to_group(unzip_task)

        src_nodes[dep_name] = src_node


class deps_ctx(BuildContext):
    cmd = 'get_deps'
    fun = 'get_deps'


def options(ctx):
    ctx.load('compiler_c compiler_cxx')


def configure(ctx):
    ctx.load('compiler_c compiler_cxx')
    if ctx.env['DEST_OS'] == 'linux':
        ctx.check_cfg(args='--cflags --libs', package='x11', uselib_store='X11', msg='Checking for X11')
        ctx.check_cfg(args='--cflags --libs', package='xrandr', uselib_store='XRANDR', msg='Checking for XRandR')
        ctx.check_cfg(args='--cflags --libs', package='openal', uselib_store='OPENAL', msg='Checking for OpenAL')
        ctx.check_cfg(args='--cflags --libs', package='sndfile', uselib_store='SNDFILE', msg='Checking for libsndfile')

def build(ctx):
    deps_node = ctx.path.make_node('deps')

    # Tetgen build
    tetgen_node = deps_node.make_node('tetgen_src').make_node('tetgen1.5.0')
    tetgen_source = ['tetgen.cxx', 'predicates.cxx']
    tetgen_source = [tetgen_node.make_node(source) for source in tetgen_source]
    tetgen_include_node = tetgen_node
    ctx.stlib(
        source       = tetgen_source,
        target       = 'tetgen',
        includes     = [tetgen_include_node],
        cflags       = ['-w', '-O3'],
        defines      = 'TETLIBRARY'
    )

    # Freetype build
    freetype_node = deps_node.make_node('freetype_src').make_node('freetype-2.5.2')
    freetype_source = ['src/base/ftsystem.c', 'src/base/ftinit.c', 'src/base/ftdebug.c', 'src/base/ftbase.c', 'src/base/ftbbox.c', 'src/base/ftglyph.c', 'src/base/ftbdf.c', 'src/base/ftbitmap.c', 'src/base/ftcid.c', 'src/base/ftfstype.c', 'src/base/ftgasp.c', 'src/base/ftgxval.c', 'src/base/ftlcdfil.c', 'src/base/ftmm.c', 'src/base/ftotval.c', 'src/base/ftpatent.c', 'src/base/ftpfr.c', 'src/base/ftstroke.c', 'src/base/ftsynth.c', 'src/base/fttype1.c', 'src/base/ftwinfnt.c', 'src/base/ftxf86.c', 'src/bdf/bdf.c', 'src/cff/cff.c', 'src/cid/type1cid.c', 'src/pcf/pcf.c', 'src/pfr/pfr.c', 'src/sfnt/sfnt.c', 'src/truetype/truetype.c', 'src/type1/type1.c', 'src/type42/type42.c', 'src/winfonts/winfnt.c', 'src/raster/raster.c', 'src/smooth/smooth.c', 'src/autofit/autofit.c', 'src/cache/ftcache.c', 'src/gzip/ftgzip.c', 'src/lzw/ftlzw.c', 'src/bzip2/ftbzip2.c', 'src/gxvalid/gxvalid.c', 'src/otvalid/otvalid.c', 'src/psaux/psaux.c', 'src/pshinter/pshinter.c', 'src/psnames/psnames.c']
    freetype_source = [freetype_node.make_node(source) for source in freetype_source]
    freetype_include_node = freetype_node.make_node('include')
    ctx.stlib(
        source       = freetype_source,
        target       = 'freetype',
        includes     = [freetype_include_node],
        cflags       = ['-w', '-O3'],
        defines      = 'FT2_BUILD_LIBRARY'
    )

    # GLEW build
    glew_node = deps_node.make_node('glew_src').make_node('glew-1.10.0')
    glew_include_node = glew_node.make_node('include')
    ctx.stlib(
        source       = glew_node.ant_glob('src/glew.c'),
        target       = 'glew',
        includes     = [glew_include_node],
        cflags       = ['-w', '-O3'],
        defines      = 'GLEW_STATIC'
    )

    # SFML build
    sfml_node =  deps_node.make_node('sfml_src').make_node('SFML-2.1')
    sfml_source = sfml_node.ant_glob('src/SFML/Audio/*.cpp') + \
                  sfml_node.ant_glob('src/SFML/Graphics/**/*.cpp') + \
                  sfml_node.ant_glob('src/SFML/System/*.cpp') + \
                  sfml_node.ant_glob('src/SFML/Window/*.cpp')
    sfml_includes =  ['include', 'src']
    sfml_uselibs = []
    if ctx.env['DEST_OS'] == 'linux':
        sfml_source = sfml_source + sfml_node.ant_glob('src/SFML/System/Unix/*.cpp') + \
                                    sfml_node.ant_glob('src/SFML/Window/Linux/*.cpp')
        sfml_uselibs.append('OPENAL')
    if ctx.env['DEST_OS'] == 'win32':
        sfml_source = sfml_source + sfml_node.ant_glob('src/SFML/System/Win32/*.cpp') + \
                                    sfml_node.ant_glob('src/SFML/Window/Win32/*.cpp')
        sfml_includes.extend(['extlibs/headers/AL', 'extlibs/headers', 'extlibs/headers/libsndfile/windows', 'extlibs/headers/jpeg', 'extlibs/headers/libfreetype/windows'])
    sfml_includes = [sfml_node.abspath() + '/' + include for include in sfml_includes]
    sfml_includes = ['/usr/include/AL/', freetype_include_node, glew_include_node] + sfml_includes
    ctx.stlib(
        source       = sfml_source,
        target       = 'sfml',
        defines      = 'SFML_STATIC UNICODE _UNICODE GLEW_STATIC',
        includes     = sfml_includes,
        uselib       = sfml_uselibs,
        cxxflags     = ['-w', '-O3']
    )

    # TGUI build
    tgui_node = deps_node.make_node('tgui_src').make_node('TGUI-0.6.6')
    ctx.env.TGUI_CONFIG_IN = tgui_node.make_node('include').make_node('TGUI').make_node('Config.hpp.in').abspath()
    ctx.env.TGUI_CONFIG_OUT = tgui_node.make_node('include').make_node('TGUI').make_node('Config.hpp').abspath()
    ctx(rule='sed -e s/@MAJOR_VERSION@/0/ -e s/@MINOR_VERSION@/6/ -e s/@PATCH_VERSION@/6/ <${TGUI_CONFIG_IN} >${TGUI_CONFIG_OUT}')
    tgui_source = tgui_node.ant_glob('src/TGUI/**/*.cpp')
    ctx.stlib(
        source       = tgui_source,
        target       = 'tgui',
        includes     = [tgui_node.make_node('include'), sfml_node.make_node('include')],
        defines      = ['TGUI_USE_STATIC_STD_LIBS', 'SFML_STATIC_LIBRARIES'],
        use          = 'sfml',
        cxxflags     = ['-w', '-O3', '--std=gnu++0x']
    )

    sources = ['main.cpp']
    uselibs = []
    defines = ['SFML_STATIC', 'UNICODE', '_UNICODE', 'GLEW_STATIC']
    libs = ['jpeg', 'sndfile']
    stlibs = []
    stlibpath = []
    includes = [tetgen_include_node, glew_include_node, sfml_node.make_node('include'), 'deps/glm_src/glm', tgui_node.make_node('include'), 'src']

    if ctx.env['DEST_OS'] == 'linux':
        uselibs.extend(['X11', 'XRANDR', 'OPENAL', 'SNDFILE'])
        libs.extend(['dl', 'GL'])
    if ctx.env['DEST_OS'] == 'win32':
        stlibpath.append(sfml_node.abspath() + '/extlibs/libs-mingw/x86/')
        libs.extend(['opengl32', 'Gdi32', 'Winmm'])

    libs.extend(['stdc++', 'm', 'rt', 'dl', 'pthread'])
    src_files = [
        'src/main.cpp',

        'src/render/Shader.cpp',
        'src/render/Renderable.cpp',
        'src/render/TetrahedralViewer.cpp',

        'src/model/IndexedFaceSet.cpp',

        'src/tetmesh/tetmesh.cpp',

        'src/util/geometry.cpp'
    ]
    ctx.program(
        source       = ' '.join(src_files),
        target       = 'idsc',
        use          = ['sfml', 'freetype', 'glew', 'tetgen', 'tgui'],
        uselib       = uselibs,

        defines      = defines,

        includes     = includes,

        lib          = libs,

        stlib        = stlibs,
        stlibpath    = stlibpath,

        linkflags    = ['-static-libstdc++', '-static-libgcc'],

        cxxflags     = ['-Wno-write-strings', '-Wall', '-O0', '-c', '-ggdb', '--std=gnu++11']
    )



