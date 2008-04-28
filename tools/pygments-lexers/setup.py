from setuptools import find_packages, setup

setup(
    name='warzone-pygments',
    version='0.1',
    packages=find_packages(exclude=['*.tests*']),
    author="Giel van Schijndel",
    author_email="me@mortis.eu",
    description="This plugin adds the capability to Pygments to lex Warzone Resource Files (WRF).",
    license="GPL",
    url="http://wz2100.net/",
    entry_points = """
        [pygments.lexers]
        wrflexer = warzone:WRFLexer
        strreslexer = warzone:STRRESLexer
        wzlevellexer = warzone:LevelLexer
    """
)
