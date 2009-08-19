# vim: set et sts=4 sw=4 encoding=utf8:
# -*- coding: utf-8 -*-

import re
from trac.admin.web_ui import _save_config as save_config

__all__ = ['get_cc_recipients', 'set_cc_recipients']

_split_args = re.compile(r',\s*').split

def get_cc_recipients(config):
    ccrcpts = _split_args(config.get('notification', 'cc_on_newticket'))

    # Make sure that empty strings result in empty lists
    if len(ccrcpts) == 1 and ccrcpts[0] == "":
        return []

    ccrcpts.sort()
    return ccrcpts

def set_cc_recipients(config, req, log, ccrcpts):
    config.set('notification', 'cc_on_newticket', ', '.join(ccrcpts))
    save_config(config, req, log)

    return get_cc_recipients(config)
