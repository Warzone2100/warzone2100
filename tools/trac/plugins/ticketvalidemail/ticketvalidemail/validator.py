# vim: set et sts=4 sw=4 encoding=utf8:
# -*- coding: utf-8 -*-

import rfc822
from email.utils import parseaddr
import re
from trac.core import *
from trac.ticket import ITicketManipulator

class TicketValidEmail(Component):
    """Extends Trac to only accept anonymous tickets when the reporter name is
    a valid RFC822 e-mail address.
    """

    implements(ITicketManipulator)

    reject_emails_re = re.compile(r'^\w+@example\.(org|net|com)$')

    # ITicketManipulator methods
    def prepare_ticket(self, req, ticket, fields, actions):
        """Not currently called, but should be provided for future
        compatibility."""
        raise NotImplementedError

    def validate_ticket(self, req, ticket):
        """Validate a ticket after it's been populated from user input.

        Must return a list of `(field, message)` tuples, one for each problem
        detected. `field` can be `None` to indicate an overall problem with the
        ticket. Therefore, a return value of `[]` means everything is OK."""

        mail = parseaddr(ticket['reporter'])[1]
        if self.reject_emails_re.match(mail.lower()):
            return [('reporter', '"%s" isn\'t acceptable as e-mail address' % (mail))]
        elif req.authname and req.authname != 'anonymous' or rfc822.valid(ticket['reporter']):
            return []
        else:
            return [(None, 'Either use a valid reporter address or log in.'),
                    ('reporter', \
                'should contain a valid e-mail address. E.g. "Nickname <user@example.org>"')]
