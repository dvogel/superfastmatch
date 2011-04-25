from paver.easy import *
import paver.doctools
from paver.setuputils import setup

CURRENT_VERSION="0.2"

setup(
    name="superfastmatch",
    packages=['superfastmatch','superfastmatch.django'],
    version=CURRENT_VERSION,
    url="http://mediastandardstrust.github.com/superfastmatch",
    author="Media Standards Trust",
    author_email="donovanhide@gmail.com"
)

options(
    minilib=Bunch( 
        extra_files=['doctools', 'virtual'] 
    ),
    sphinx=Bunch(
        builddir=".build"
    ),
    docs_env=Bunch(
        dest_dir=".env/docs_env",
        script_name='.env/docs_env.py',
        requirements="requirements/docs.txt",
        no_site_packages=True
    ),
    examples_env=Bunch(
        dest_dir=".env/examples_env",
        script_name='.env/examples_env.py',
        requirements="requirements/examples.txt",
        no_site_packages=True
    )
)

def bash(command):
    sh("bash -c '%s'"%command)
    
def pip_install(env_dir,requirements):
    bash("source %s/bin/activate && pip install -r %s"%(env_dir,requirements))

def pip_upgrade(env_dir, package):
    bash("source %s/bin/activate && pip install --upgrade %s"%(env_dir,package))

@task
@needs('docs','generate_setup', 'minilib', 'setuptools.command.sdist')
def sdist():
    """Overrides sdist to make sure that our docs and setup.py are generated."""
    pass

@task
@needs('paver.doctools.html')
def html(options):
    """Build the docs and put them into our package."""
    destdir = path('superfastmatch/docs')
    destdir.rmtree()
    builtdocs = path("docs") / options.builddir / "html"
    builtdocs.move(destdir)

@task
def docs(options):
    """Build environment for creating docs and then build the docs"""
    path(options.docs_env.dest_dir).makedirs()
    options.virtualenv=options.docs_env
    call_task("paver.virtual.bootstrap")
    pip_install(options.virtualenv.dest_dir,options.virtualenv.requirements)
    call_task("paver.doctools.doc_clean")
    bash("python %s && source %s/bin/activate && paver html" % (options.docs_env.script_name,options.docs_env.dest_dir))

@task
def github_docs(options):
    """Build the docs and upload to GitHub - You'll need to be a committer!"""
    call_task("docs")
    bash("git checkout gh-pages && \
        cp -r superfastmatch/docs/ . && \
        git add . && \
        git commit . -m 'Rendered documentation for Github Pages.' && \
        git push origin gh-pages && \
        git checkout master" % options)

@task 
@needs('sdist')
def examples(options):
    """Build examples environment"""
    path(options.examples_env.dest_dir).makedirs()
    options.virtualenv=options.examples_env
    call_task("paver.virtual.bootstrap")
    pip_install(options.virtualenv.dest_dir,options.virtualenv.requirements)
    pip_upgrade(options.virtualenv.dest_dir,'dist/superfastmatch-%s.tar.gz'%CURRENT_VERSION)

@task
@needs('examples')
def legislation(options):
    """Runs legislation example"""
    bash("source %s/bin/activate && \
        cd examples/legislation && \
        ./manage.py syncdb && \
        ./manage.py runserver" % options.examples_env.dest_dir)

