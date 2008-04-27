from pygments.lexer import RegexLexer
from pygments.token import *

class WRFLexer(RegexLexer):
    name = 'WRF'
    aliases = ['wrf']
    filenames = ['*.wrf']
    mimetypes = ['text/x-wrf']

    tokens = {
        'root': [
            (r'/\*', Comment.Multiline, 'comment'),
            (r'\bdirectory\b', Keyword),
            (r'\bfile\b', Keyword, 'file_line'),
            (r'"[^"]*"', String),
            (r'[ \t\n\x0d\x0a]+', Text),
        ],
        'comment': [
            (r'[^*]+', Comment.Multiline),
            (r'\*/', Comment.Multiline, '#pop'),
            (r'\*[^/]', Comment.Multiline),
        ],
        'file_line': [
            (r'[ \t\n\x0d\x0a]+', Text),
            (r'[^ \t\n\x0d\x0a]+', Literal, '#pop'),
        ]
    }
