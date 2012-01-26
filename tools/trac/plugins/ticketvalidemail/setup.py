#!/usr/bin/env python
# vim: set et sts=4 sw=4 encoding=utf8:
# -*- coding: utf-8 -*-

from setuptools import setup

setup(
    name = 'TicketValidEmail',
    version = '0.2',

    packages = [
        'ticketvalidemail',
    ],

    install_requires = ['trac>=0.11'],
    extras_require = {
        'TracSpamFilter': ['tracspamfilter>0.4.7']
    },

    author = 'Giel van Schijndel',
    author_email = 'me@mortis.eu',
    description = 'Extends Trac to only accept anonymous tickets when the reporter name is a valid RFC822 e-mail address.',
    license = 'BSD',
    keywords = 'trac plugin ticket create reporter valid rfc822 e-mail',
    url = 'https://github.com/Warzone2100/warzone2100/tree/master/tools/trac/plugins/ticketvalidemail',
    classifiers = [
        'Framework :: Trac',
        'License :: OSI Approved :: BSD License',
    ],

    entry_points = {
        'trac.plugins': [
            'ticketvalidemail.validator = ticketvalidemail.validator',
            'ticketvalidemail.spamfilter = ticketvalidemail.spamfilter[TracSpamFilter]',
        ],
    },
)
