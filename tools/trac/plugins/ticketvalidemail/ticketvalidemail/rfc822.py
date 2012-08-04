# vim: set et sts=4 sw=4 fileencoding=utf8:
#
# Copyright (C) 2001-2002  Paul Warren <pdw@ex-parrot.com>
#                          - original Perl version
# Copyright (C) 2010       Giel van Schijndel
#                          - Python conversion
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is furnished to do
# so, subject to the following conditions: The above copyright notice and this
# permission notice shall be included in all copies or substantial portions of
# the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

import re

__all__ = ['make_rfc822re', 'strip_comments', 'valid', 'validlist']

# Allowed whitespace
lwsp = "(?:(?:\\r\\n)?[ \\t])"

# Allowed characters
chars = re.compile(r'^[\000-\177]*$')

def make_rfc822re():
    """Returns a string that represents a regex that will match all valid
    RFC822 addresses.
    """
#   Basic lexical tokens are specials, domain_literal, quoted_string, atom, and
#   comment.  We must allow for lwsp (or comments) after each of these.
#   This regexp will only work on addresses which have had comments stripped
#   and replaced with lwsp.

    specials = '()<>@,;:\\\\".\\[\\]'
    controls = '\\000-\\037\\177'

    dtext = "[^\\[\\]\\r\\\\]"
    domain_literal = "\\[(?:" + dtext + "|\\\\.)*\\]" + lwsp + "*"

    quoted_string = "\"(?:[^\\\"\\r\\\\]|\\\\.|" + lwsp + ")*\"" + lwsp + "*"

#   Use zero-width assertion to spot the limit of an atom.  A simple
#   $lwsp* causes the regexp engine to hang occasionally.
    atom = "[^" + specials + " " + controls + "]+(?:" + lwsp + "+|\\Z|(?=[\\[\"" + specials + "]))"
    word = "(?:" + atom + "|" + quoted_string + ")"
    localpart = word + "(?:\\." + lwsp + "*" + word + ")*"

    sub_domain = "(?:" + atom + "|" + domain_literal + ")"
    domain = sub_domain + "(?:\\." + lwsp + "*" + sub_domain + ")*"

    addr_spec = localpart + "@" + lwsp + "*" + domain + ""

    phrase = word + "*"
    route = "(?:@" + domain + "(?:,@" + lwsp + "*" + domain + ")*:" + lwsp + "*)"
    route_addr = "\\<" + lwsp + "*" + route + "?" + addr_spec + "\\>" + lwsp + "*"
    mailbox = "(?:" + addr_spec + "|" + phrase + "" + route_addr + ")"

    group = "" + phrase + ":" + lwsp + "*(?:" + mailbox + "(?:,\\s*" + mailbox + ")*)?;\\s*"
    address = "(?:" + mailbox + "|" + group + ")"

    return lwsp + "*" + address

comment_re = re.compile(r"""^((?:[^"\\]|\\.)*
                              (?:"(?:[^"\\]|\\.)*"(?:[^"\\]|\\.)*)*)
                              \((?:[^()\\]|\\.)*\)""", re.DOTALL | re.VERBOSE)

def strip_comments(email):
    """Strips the comment portions from e-mail addresses"""
    #   Recursively remove comments, and replace with a single space.  The
    #   simpler regexps in the Email Addressing FAQ are imperfect - they will
    #   miss escaped chars in atoms, for example.

    while comment_re.match(email):
        email = comment_re.sub(r'\1 ', email, 1)

    return email

rfc822re = re.compile('^' + make_rfc822re() + '$', re.DOTALL)

def valid(email):
    """Returns True if the given string is a valid RFC822 address"""
    email = strip_comments(email)

    return bool(chars.match(email) and rfc822re.match(email))

list_re = re.compile('^(?:' + make_rfc822re() + ')?(?:,(?:' + make_rfc822re() + ')?)*$', re.DOTALL)
listitem_re = re.compile('(?:^|,' + lwsp + '*)(' + make_rfc822re() + ')', re.DOTALL)

def validlist(emaillist):
    """If the given string is an RFC822 valid list of addresses a list of
    strings is returned. Each string representing a single valid e-mail
    address. Otherwise None is returned.
    """
    emaillist = strip_comments(emaillist)

    if chars.match(emaillist) and list_re.match(emaillist):
        return listitem_re.findall(emaillist)

# Unit testing...
if __name__ == "__main__":
    valids = [r"""abigail@example.com""",
              r"""abigail@example.com """,
              r""" abigail@example.com""",
              r"""abigail @example.com""",
              r"""*@example.net""",
              r""""\""@foo.bar""",
              r"""fred&barny@example.com""",
              r"""---@example.com""",
              r"""foo-bar@example.net""",
              r""""127.0.0.1"@[127.0.0.1]""",
              r"""Abigail <abigail@example.com>""",
              r"""Abigail<abigail@example.com>""",
              r"""Abigail<@a,@b,@c:abigail@example.com>""",
              r""""This is a phrase"<abigail@example.com>""",
              r""""Abigail "<abigail@example.com>""",
              r""""Joe & J. Harvey" <example @Org>""",
              r"""Abigail <abigail @ example.com>""",
              r"""Abigail made this <  abigail   @   example  .    com    >""",
              r"""Abigail(the bitch)@example.com""",
              r"""Abigail <abigail @ example . (bar) com >""",
              r"""Abigail < (one)  abigail (two) @(three)example . (bar) com (quz) >""",
              r"""Abigail (foo) (((baz)(nested) (comment)) ! ) < (one)  abigail (two) @(three)example . (bar) com (quz) >""",
              r"""Abigail <abigail(fo\(o)@example.com>""",
              r"""Abigail <abigail(fo\)o)@example.com>""",
              r"""(foo) abigail@example.com""",
              r"""abigail@example.com (foo)""",
              r""""Abi\"gail" <abigail@example.com>""",
              r"""abigail@[example.com]""",
              r"""abigail@[exa\[ple.com]""",
              r"""abigail@[exa\]ple.com]""",
              r"""":sysmail"@  Some-Group. Some-Org""",
              r"""Muhammed.(I am  the greatest) Ali @(the)Vegas.WBA""",
              r"""mailbox.sub1.sub2@this-domain""",
              r"""sub-net.mailbox@sub-domain.domain""",
              r"""name:;""",
              r"""':;""",
              r"""name:   ;""",
              r"""Alfred Neuman <Neuman@BBN-TENEXA>""",
              r"""Neuman@BBN-TENEXA""",
              r""""George, Ted" <Shared@Group.Arpanet>""",
              r"""Wilt . (the  Stilt) Chamberlain@NBA.US""",
              r"""Cruisers:  Port@Portugal, Jones@SEA;""",
              r"""$@[]""",
              r"""*()@[]""",
              r""""quoted ( brackets" ( a comment )@example.com""",
    ]

    invalids = [r"""Just a string""",
                r"""string""",
                r"""(comment)""",
                r"""()@example.com""",
                r"""fred(&)barny@example.com""",
                r"""fred\ barny@example.com""",
                r"""Abigail <abi gail @ example.com>""",
                r"""Abigail <abigail(fo(o)@example.com>""",
                r"""Abigail <abigail(fo)o)@example.com>""",
                r""""Abi"gail" <abigail@example.com>""",
                r"""abigail@[exa]ple.com]""",
                r"""abigail@[exa[ple.com]""",
                r"""abigail@[exaple].com]""",
                r"""abigail@""",
                r"""@example.com""",
                r"""phrase: abigail@example.com abigail@example.com ;""",
                u"""invalid£char@example.com""",
    ]

    lists = [
        ('abc@foo.com, abc@blort.foo', ['abc@foo.com', 'abc@blort.foo']),
        ('abc@foo.com, abcblort.foo', None),
        ('', []),
    ]

    for n, address in enumerate(valids):
        if valid(address):
            print "ok %d" % (n + 1)
        else:
            print "not ok %d" % (n + 1)
            print "#  [VALID: %s] " % (address)

    for n, address in enumerate(invalids):
        if not valid(address):
            print "ok %d" % (n + 1)
        else:
            print "not ok %d" % (n + 1)
            print "#  [INVALID: %s] " % (address)
    for n, (inp, retval) in enumerate(lists):
        if validlist(inp) == retval:
            print "ok %d" % (n + 1)
        else:
            print "not ok %d" % (n + 1)
            print "# [validlist: %s]" % (inp)
