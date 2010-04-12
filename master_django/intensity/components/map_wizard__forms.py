
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

from fnmatch import fnmatch

from django import forms

from intensity.models import AssetInfo


map_pattern = 'base/*.tar.gz'

class MapWizardForm(forms.Form):
    location = forms.CharField(max_length=200, initial='base/???.tar.gz')
    original = forms.ModelChoiceField(queryset=AssetInfo.objects.order_by('location'), initial=AssetInfo.get_emptymap().id)
    requisition = forms.BooleanField(initial=True, required=False)

    def __init__(self, *args, **kwargs):
        super(MapWizardForm, self).__init__(*args, **kwargs)
        self.fields['original'].queryset = queryset=AssetInfo.objects.filter(location__startswith='base/').filter(location__endswith='.tar.gz').order_by('location')

    def clean_location(self):
        location = self.cleaned_data['location']
        if not fnmatch(location, map_pattern):
            raise forms.ValidationError('''A map's location must always be in the form "base/MAPNAME.tar.gz"''')

        if len( AssetInfo.objects.filter(location=location) ) != 0:
            raise forms.ValidationError('''A map already exists with that location''')

        return location

