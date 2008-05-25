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
            (r'//.*?$', Comment.Singleline),
            (r'\bdirectory\b', Keyword),
            (r'\bfile\b', Keyword, 'resource_line'),
            (r'\bdatabase\b', Keyword),
            (r'\btable\b', Keyword, 'resource_line'),
            (r'"[^"]*"', String),
            (r'[ \t\n\x0d\x0a]+', Text),
        ],
        'comment': [
            (r'[^*]+', Comment.Multiline),
            (r'\*/', Comment.Multiline, '#pop'),
            (r'\*[^/]', Comment.Multiline),
        ],
        'resource_line': [
            (r'[ \t\n\x0d\x0a]+', Text),
            (r'\b.+?\b', Literal, '#pop'),
        ]
    }

class STRRESLexer(RegexLexer):
    name = 'STRRES'
    aliases = ['strres']
    filenames = ['*.txt']
    mimetypes = ['text/x-strres']

    tokens = {
        'root': [
            (r'/\*', Comment.Multiline, 'comment'),
            (r'//.*?$', Comment.Singleline),
            (r'"[^"]*"', String),
            (r'[_a-zA-Z][-0-9_a-zA-Z]*', Literal),
            (r'[ \t\n\x0d\x0a]+', Text),
            (r'\(', Punctuation, '#push'),
            (r'\)', Punctuation, '#pop'),
        ],
        'comment': [
            (r'[^*]+', Comment.Multiline),
            (r'\*/', Comment.Multiline, '#pop'),
            (r'\*[^/]', Comment.Multiline),
        ],
    }

class LevelLexer(RegexLexer):
    name = 'WZLevel'
    aliases = ['wzlevel']
    filenames = ['*.lev']
    mimetypes = ['text/x-wzlev']

    tokens = {
        'root': [
            (r'/\*', Comment.Multiline, 'comment'),
            (r'//.*?$', Comment.Singleline),
            (r'\b(level|campaign|camstart|camchange|dataset|expand|expand_limbo|between|miss_keep|miss_keep_limbo|miss_clear)\b', Keyword, 'dataset_line'),
            (r'\b(players|type)\b', Keyword, 'integer_line'),
            (r'\b(data|game)\b', Keyword),
            (r'"[^"]*"', String),
            (r'[ \t\n\x0d\x0a]+', Text),
        ],
        'comment': [
            (r'[^*]+', Comment.Multiline),
            (r'\*/', Comment.Multiline, '#pop'),
            (r'\*[^/]', Comment.Multiline),
        ],
        'dataset_line': [
            (r'[ \t\n\x0d\x0a]+', Text),
            (r'\b[_a-zA-Z][-0-9_a-zA-Z]+\b', Literal, '#pop'),
        ],
        'integer_line': [
            (r'[ \t\n\x0d\x0a]+', Text),
            (r'-?[0-9]+', Number, '#pop'),
        ]
    }
