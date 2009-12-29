
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

