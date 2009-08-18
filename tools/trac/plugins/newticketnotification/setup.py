#!/usr/bin/env python
# vim: set et sts=4 sw=4 encoding=utf8:
# -*- coding: utf-8 -*-

from setuptools import setup

setup(
    name = 'NewTicketNotification',
    version = '0.1',

    packages = ['newticketnotification'],

    install_requires = ['trac>=0.11'],

    author = 'Giel van Schijndel',
    author_email = 'me@mortis.eu',
    description = 'Extends Trac to notify a configured set of e-mail addresses upon ticket creation.',
    license = 'BSD',
    keywords = 'trac plugin ticket create notify e-mail',
    classifiers = [
        'Framework :: Trac',
        'License :: OSI Approved :: BSD License',
    ],

    entry_points = {
        'trac.plugins': [
            'newticketnotification.main = newticketnotification.main',
        ],
    },
)
