
#=============================================================================
# Copyright (C) 2008 Alon Zakai ('Kripken') kripkensteiner@gmail.com
#
# This file is part of the Intensity Engine project,
#    http://www.intensityengine.com
#
# The Intensity Engine is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, version 3.
#
# The Intensity Engine is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with the Intensity Engine.  If not, see
#     http://www.gnu.org/licenses/
#     http://www.gnu.org/licenses/agpl-3.0.html
#=============================================================================


import uuid, logging

from django.http import HttpResponseRedirect
from django.views.generic.simple import direct_to_template
from django.shortcuts import render_to_response
from django.views.decorators.cache import never_cache
from django.template import RequestContext
from django.http import HttpResponseRedirect
from django.contrib.auth.models import User
from django.contrib.auth.decorators import login_required

from intensity.register.models import SignupForNotify
from intensity.register.forms import UserAccountCreationForm, PasswordResetForm, PasswordChangeForm, SignupForNotifyForm
from intensity.signals import multiple_send, send_email
from intensity.views import getpost_form
import intensity.conf as intensity_conf


@never_cache
def register(request, template_name='registration/register.html'):
    if intensity_conf.get('Accounts', 'allow_registrations', '') != '1':
        return HttpResponseRedirect('/accounts/register/signup_for_notify/')

    def on_valid(form):
        logging.info('Creating user: ' + form.cleaned_data['username'])

        User.objects.create_user(
            username=form.cleaned_data['username'],
            password=form.cleaned_data['password1'],
            email=form.cleaned_data['email'],
        )

    return getpost_form(request, 'registration/register.html', UserAccountCreationForm, on_valid, '/accounts/register/finish/', form_params={'request': request})


@never_cache
def reset_password(request):
    def on_valid(form):
        email = form.cleaned_data['email']
        assert(email != '')
        try:
            user = User.objects.get(email=email)

            logging.info('Resetting password for user: ' + user.username)

            # Set new password
            password = str(uuid.uuid4()).replace('-', '').replace('0', 'g')[0:9]
            user.set_password(password)
            user.save()

            send_email.send(
                None,
                recipient=email,
                subject='Syntensity password reset',
                body='''
    Our system received a request to reset the password for your account at Syntensity.

    Your password is now set to: %s

    Thank you for using Syntensity.
                     ''' % password,
            )
        except User.DoesNotExist:
            pass # Still show message, do not give hints as to validity of account or not

    return getpost_form(request, 'registration/password_reset.html', PasswordResetForm, on_valid, '/accounts/resetpassword/finish/', form_params={'request': request})


@never_cache
@login_required
def change_password(request):
    def on_valid(form):
        logging.info('Changing password for user: ' + request.user.username)

        password = form.cleaned_data['password1']
        request.user.set_password(password)
        request.user.save()
        request.session['message'] = 'Password successfully changed'
    return getpost_form(request, 'registration/password_change.html', PasswordChangeForm, on_valid, '/tracker/account/')


def signup_for_notify(request):
    def on_valid(form):
        email = form.cleaned_data['email']
        logging.info('Person asked to be notified when registrations open: ' + email)
        signup = SignupForNotify.objects.create(email=email)
    return getpost_form(request, 'registration/signup_for_notify.html', SignupForNotifyForm, on_valid, '/accounts/register/signup_for_notify/finish/')

