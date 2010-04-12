
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

from django import forms
from django.forms import Form, ModelForm

from intensity.models import UserAccount, ServerInstance, AssetInfo, Activity
from intensity.widgets import CheckboxSelectMultipleChecked
from intensity.utility import check_ascii


class ServerInstanceUpdateForm(ModelForm):
    class Meta:
        model = ServerInstance
        fields = ('user_interface', 'admin_interface', 'error_log', 'players', 'max_players')

class AssetForm(ModelForm):
    class Meta:
        model = AssetInfo
        fields = ('location', 'type_x', 'comment')
        __additional_m2m = ('owners', 'dependencies') # Workaround for Django issue

    owners = forms.ModelMultipleChoiceField(queryset=UserAccount.objects.none(), widget=CheckboxSelectMultipleChecked())
    add_owner = forms.ModelChoiceField(queryset=UserAccount.objects.order_by('nickname'), required=False)

    dependencies = forms.ModelMultipleChoiceField(queryset=AssetInfo.objects.none(), required=False, widget=CheckboxSelectMultipleChecked())
    add_dependency = forms.ModelChoiceField(queryset=AssetInfo.objects.order_by('location'), required=False)

    def __init__(self, *args, **kwargs):
        super(AssetForm, self).__init__(*args, **kwargs)
        if self.instance is not None and self.instance.pk is not None:
            # Can only *remove* out of these sets (we don't show all the potentially huge number)
            self.fields['owners'].queryset = self.instance.owners
            self.fields['dependencies'].queryset = self.instance.dependencies

    def clean_location(self):
        self.instance.location = self.cleaned_data['location']
        return check_ascii(self.instance.location)

    def clean_comment(self):
        self.instance.comment = self.cleaned_data['comment']
        return check_ascii(self.instance.comment)

    def clean_owners(self):
        self.instance.owners = self.cleaned_data['owners']
        return self.instance.owners

    def clean_dependencies(self):
        self.instance.dependencies = self.cleaned_data['dependencies']
        return self.instance.dependencies

    def clean_add_owner(self):
        new_owner = self.cleaned_data['add_owner']
        if new_owner is not None:
            self.instance.owners.add(new_owner)
        return new_owner

    def clean_add_dependency(self):
        new_dep = self.cleaned_data['add_dependency']
        if new_dep is not None:
            self.instance.dependencies.add(new_dep)
        return new_dep


class ActivityForm(ModelForm):
    class Meta:
        model = Activity
        fields = ('name',)

    def clean_name(self):
        self.instance.name = self.cleaned_data['name']
        return check_ascii(self.instance.name)

