
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


from django.utils.translation import ugettext_lazy as _
from django.contrib.auth.models import User
from django import forms, dispatch
from django.utils.safestring import SafeString

from intensity.signals import singleton_send, multiple_send, prepare_security_check, verify_security_check


def clean_password2(self):
    password1 = self.cleaned_data.get("password1", "")
    password2 = self.cleaned_data["password2"]
    if password1 != password2:
        raise forms.ValidationError(_("The two password fields didn't match."))
    return password2

##

class SecurityCheckedForm(forms.Form):
    def prepare_security_checks(self, errors=[]):
        checks = multiple_send(prepare_security_check, None, errors = errors)
        checks_html = '<br>'.join(checks)
        self.security_checks = SafeString(checks_html)

    def __init__(self, *args, **kwargs):
        self.request = kwargs['request'] # Needed for security checks, now and in clean()
        del kwargs['request']
        super(SecurityCheckedForm, self).__init__(*args, **kwargs)

        if self.request.method == 'GET':
            self.prepare_security_checks()

    def clean(self):
        # Security checks
        verifications = multiple_send(verify_security_check, None, request=self.request)
        security_errors = filter(lambda x: x is not True, verifications)

        self.prepare_security_checks(security_errors) # Not strictly necessary if all other fields were ok

        if len(security_errors) != 0:
            raise forms.ValidationError(_('Security checks not passed. See reasons below.'))

        return super(SecurityCheckedForm, self).clean()


class UserAccountCreationForm(SecurityCheckedForm):
    username = forms.RegexField(label=_("Username"), max_length=30, regex=r'^\w+$',
        help_text = _("Required. 30 characters or fewer. Alphanumeric characters only (letters, digits and underscores)."),
        error_message = _("This value must contain only letters, numbers and underscores."))
    password1 = forms.CharField(label=_("Password"), widget=forms.PasswordInput)
    password2 = forms.CharField(label=_("Password confirmation"), widget=forms.PasswordInput)
    email = forms.EmailField()
    terms = forms.BooleanField()

    class Meta:
        model = User
        fields = ("username",)

    def clean_username(self):
        username = self.cleaned_data["username"]
        try:
            User.objects.get(username=username)
        except User.DoesNotExist:
            return username
        raise forms.ValidationError(_("A user with that username already exists."))

    clean_password2 = clean_password2


class PasswordResetForm(SecurityCheckedForm):
    email = forms.EmailField()

class PasswordChangeForm(forms.Form):
    password1 = forms.CharField(label=_("Password"), widget=forms.PasswordInput)
    password2 = forms.CharField(label=_("Password confirmation"), widget=forms.PasswordInput)

    clean_password2 = clean_password2

class SignupForNotifyForm(forms.Form):
    email = forms.EmailField()

