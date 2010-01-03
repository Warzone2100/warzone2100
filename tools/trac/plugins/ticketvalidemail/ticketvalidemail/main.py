# vim: set et sts=4 sw=4 encoding=utf8:
# -*- coding: utf-8 -*-

import address
from trac.core import *
from trac.ticket import ITicketManipulator

class TicketValidEmail(Component):
    """Extends Trac to only accept anonymous tickets when the reporter name is
    a valid RFC822 e-mail address.
    """

    implements(ITicketManipulator)

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
        if req.authname and req.authname != 'anonymous' or address.valid(ticket['reporter']):
            return []
        else:
            return [('reporter', \
                'should contain a valid e-mail address. E.g. "Nickname <user@example.org>"')]
