# vim: set et sts=4 sw=4 encoding=utf8:
# -*- coding: utf-8 -*-

from model import *

from trac.admin.api import IAdminPanelProvider
from trac.core import *
from trac.util.translation import _
from trac.web.chrome import ITemplateProvider

class NewTicketNotificationAdmin(Component):
    """Allows to setup a list of e-mail addresses to be notified upon ticket
    creation.
    """

    implements(ITemplateProvider, IAdminPanelProvider)

    # ITemplateProvider methods
    def get_htdocs_dirs(self):
        """Return a list of directories with static resources (such as style
        sheets, images, etc.)

        Each item in the list must be a `(prefix, abspath)` tuple. The
        `prefix` part defines the path in the URL that requests to these
        resources are prefixed with.
        
        The `abspath` is the absolute path to the directory containing the
        resources on the local file system.
        """
        return []

    def get_templates_dirs(self):
        """Return a list of directories containing the provided template
        files.
        """
        from pkg_resources import resource_filename
        return [resource_filename(__name__, 'templates')]

    # IAdminPanelProvider
    def get_admin_panels(self, req):
        """Return a list of available admin panels.
        
        The items returned by this function must be tuples of the form
        `(category, category_label, page, page_label)`.
        """
        if 'TRAC_ADMIN' in req.perm:
            yield ('ticket', 'Ticket System', 'notifications', 'Notifications')

    def render_admin_panel(self, req, category, page, path_info):
        """Process a request for an admin panel.
        
        This function should return a tuple of the form `(template, data)`,
        where `template` is the name of the template to use and `data` is the
        data to be passed to the template.
        """
        req.perm.require('TRAC_ADMIN')

        ccrcpts = get_cc_recipients(self.config)

        if req.method == 'POST':
            # Add address
            if   req.args.get('add'):
                address = req.args.get('email')
                if not address:
                    raise TracError(_('Empty address provided'))

                # Add the given address to the configuration and save it
                ccrcpts = set_cc_recipients(self.config,
                                            req,
                                            self.log,
                                            sorted(set(ccrcpts) | set([address])))
            elif req.args.get('remove'):
                sel = req.args.get('sel')
                if not sel:
                    raise TracError(_('No address selected'))
                if not isinstance(sel, list):
                    sel = [sel]

                # Remove the given addresses from the configuration and save it
                ccrcpts = set_cc_recipients(self.config,
                                            req,
                                            self.log,
                                            sorted(set(ccrcpts) - set(sel)))

        return 'newticketnotification_admin.html', {
                'newticketnotification': {
                    'addresses': ccrcpts,
                    },
            }
