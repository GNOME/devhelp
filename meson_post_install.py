#!/usr/bin/env python3

import os
import subprocess
import sys

data_dir = sys.argv[1]

if not os.environ.get('DESTDIR'):
    icon_dir = os.path.join(data_dir, 'icons', 'hicolor')
    print('Update icon cache...')
    subprocess.call(['gtk-update-icon-cache', '-f', '-t', icon_dir])

    schema_dir = os.path.join(data_dir, 'glib-2.0', 'schemas')
    print('Compiling gsettings schemas...')
    subprocess.call(['glib-compile-schemas', schema_dir])
