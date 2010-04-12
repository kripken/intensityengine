
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

from __future__ import with_statement

from urlparse import urlparse

from django.test import TestCase
from django.contrib.auth.models import User

from intensity.models import UserAccount, Activity, AssetInfo, ServerInstance, classes
from intensity.signals import send_email
from intensity.register.models import SignupForNotify
import intensity.conf as intensity_conf


def get_redirect_url(response):
    url = filter(lambda x: 'Location: ' in x, str(response).split('\n'))[0].replace('Location: ', '').replace('\n', '')
    parsed = urlparse(url)
    ret = parsed.path
    if parsed.query != '':
        ret += '?' + parsed.query
    return ret


class IntensityConfSetting:
    def __init__(self, *args):
        self.args = args
    def __enter__(self):
        self.saved = intensity_conf.get(*(self.args[0:2]))
        intensity_conf.set(*(self.args))
    def __exit__(self, x, y, z):
        intensity_conf.set(*(self.args[0:2] + (self.saved,)))

class EmailSender:
    def __init__(self, sender=None):
        def nosend(sender, **kwargs): pass
        if sender is None:
            sender = nosend
        self.sender = sender
    def __enter__(self):
        send_email.connect(self.sender, weak=False)
    def __exit__(self, x, y, z):
        send_email.disconnect(self.sender)


class UserAccountTestCase(TestCase):
    def setUp(self):
        self.username = 'testuser_useraccounttest_xxx'
        self.password = 'passfail'
        self.user = User.objects.create_user(username=self.username, email='a@a.ca', password=self.password)

    def tearDown(self):
        if self.user.pk is not None: # Might have been deleted already
            self.user.delete()

    def testBasis(self):
        self.assertEquals(User.objects.get(username=self.username), self.user)

    def testAutocreate(self):
        self.assertEquals(UserAccount.objects.get(user=self.user).user, self.user)

    def testAutodelete(self):
        self.user.delete()
        def look():
            UserAccount.objects.get(user=self.user)
        self.assertRaises(UserAccount.DoesNotExist, look)

    def testLogin(self):
        self.assertEquals(self.client.login(username=self.username, password=self.password), True)
        self.client.logout()
        self.assertEquals(self.client.login(username=self.username, password=self.password + 'xyz'), False)

    def testAccountMiddleware(self):
        account = UserAccount.objects.get(user=self.user)

        self.client.login(username=self.username, password=self.password)

        response = self.client.get('/toplevel/overview/')
        for con in response.context:
            self.assertEquals(con['my_account'], account)

    def testRegistration(self):
        with IntensityConfSetting('Accounts', 'allow_registrations', ''):
            self.assertRedirects(self.client.get('/accounts/register/'), '/accounts/register/signup_for_notify/')

            self.assertRedirects(self.client.post('/accounts/register/signup_for_notify/', {
                'email': 'morbo@magnificient.com',
            }), '/accounts/register/signup_for_notify/finish/')

            self.assertEquals(len(SignupForNotify.objects.all()), 1)
            self.assertEquals(SignupForNotify.objects.all()[0].email, 'morbo@magnificient.com')

        with IntensityConfSetting('Accounts', 'allow_registrations', '0'):
            self.assertRedirects(self.client.get('/accounts/register/'), '/accounts/register/signup_for_notify/')

        with IntensityConfSetting('Accounts', 'allow_registrations', '1'):
            self.assertContains(self.client.get('/accounts/register/'), 'I agree to the <a href="#tos">Terms of Service</a> appearing below')
            self.assertContains(self.client.post('/accounts/register/'), 'This field is required.') # Fail with no info

    def testClasses(self):
        self.assertTrue(UserAccount in classes)
        self.assertTrue(Activity in classes)
        self.assertTrue(AssetInfo in classes)
        self.assertTrue(ServerInstance in classes)

    def testPasswordReset(self):
        self.assertContains(self.client.get('/accounts/resetpassword/'), 'Enter your email')
        self.assertContains(self.client.post('/accounts/resetpassword/'), 'is required')

        class Content:
            emails = []

        def adder(sender, **kwargs):
            Content.emails.append(kwargs['body'])

        with EmailSender(adder):
            account = User.objects.create_user(username='auser', password='apassword', email='testaccountzxy@syntensity.com')

            self.assertEquals(len(Content.emails), 0)

            # Require captcha, if we are using one
            if intensity_conf.get('ReCaptcha', 'public_key') != '':
                self.assertContains(self.client.post('/accounts/resetpassword/', {'email': 'idunno@sup.net'}), 'Security checks not passed.')

            with IntensityConfSetting('ReCaptcha', 'public_key', ''): # Disable captcha
                with IntensityConfSetting('ReCaptcha', 'private_key', ''): # Disable captcha
                    url = get_redirect_url(self.client.post('/accounts/resetpassword/', {'email': 'idunno@sup.net'}))
                    self.assertContains(
                        self.client.get(url),
                        'If an account exists with that email, a new password has been sent to it'
                    )
                    self.assertEquals(len(Content.emails), 0) # No such account

                    with IntensityConfSetting('Email', 'username', ''): # Disable email
                        url = get_redirect_url(self.client.post('/accounts/resetpassword/', {'email': 'testaccountzxy@syntensity.com'}))
                        self.assertContains(
                            self.client.get(url),
                            'If an account exists with that email, a new password has been sent to it'
                        )
                        self.assertEquals(len(Content.emails), 1)

            # Succeed to login with new password

            lines = Content.emails[0].split('\n')
            password = None
            for line in lines:
                ind = 'Your password is now set to: '
                if ind in line:
                    password = line[line.find(ind)+len(ind):]
            self.assertNotEquals(password, None)

            self.assertTrue(self.client.login(username='auser', password=password))


