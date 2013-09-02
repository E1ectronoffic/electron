#!/usr/bin/env python

import argparse
import errno
import glob
import os
import subprocess
import sys
import tempfile

from lib.util import *


TARGET_PLATFORM = {
  'cygwin': 'win32',
  'darwin': 'darwin',
  'linux2': 'linux',
  'win32': 'win32',
}[sys.platform]

ATOM_SHELL_VRESION = get_atom_shell_version()
NODE_VERSION = 'v0.10.15'

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
OUT_DIR = os.path.join(SOURCE_ROOT, 'out', 'Release')
DIST_DIR = os.path.join(SOURCE_ROOT, 'dist')
DIST_NAME = 'atom-shell-{0}-{1}.zip'.format(ATOM_SHELL_VRESION, TARGET_PLATFORM)


def main():
  args = parse_args()

  if not dist_newer_than_head():
    create_dist = os.path.join(SOURCE_ROOT, 'script', 'create-dist.py')
    subprocess.check_call([sys.executable, create_dist])

  bucket, access_key, secret_key = s3_config()
  upload(bucket, access_key, secret_key)
  if not args.no_update_version:
    update_version(bucket, access_key, secret_key)


def parse_args():
  parser = argparse.ArgumentParser(description='upload distribution file')
  parser.add_argument('-n', '--no-update-version',
                      help='Do not update the latest version file',
                      action='store_false')
  return parser.parse_args()


def dist_newer_than_head():
  with scoped_cwd(SOURCE_ROOT):
    try:
      head_time = subprocess.check_output(['git', 'log', '--pretty=format:%at',
                                           '-n', '1']).strip()
      dist_time = os.path.getmtime(os.path.join(DIST_DIR, DIST_NAME))
    except OSError as e:
      if e.errno != errno.ENOENT:
        raise
      return False

  return dist_time > int(head_time)


def upload(bucket, access_key, secret_key, version=ATOM_SHELL_VRESION):
  os.chdir(DIST_DIR)

  s3put(bucket, access_key, secret_key, DIST_DIR,
        'atom-shell/{0}'.format(version), [DIST_NAME])
  s3put(bucket, access_key, secret_key, DIST_DIR,
        'atom-shell/dist/{0}'.format(NODE_VERSION), glob.glob('node-*.tar.gz'))

  if TARGET_PLATFORM == 'win32':
    # Generate the node.lib.
    build = os.path.join(SOURCE_ROOT, 'script', 'build.py')
    subprocess.check_call([sys.executable, build, '-c', 'Release',
                          '-t', 'generate_node_lib'])

    # Upload the 32bit node.lib.
    node_lib = os.path.join(OUT_DIR, 'node.lib')
    s3put(bucket, access_key, secret_key, OUT_DIR,
          'atom-shell/dist/{0}'.format(NODE_VERSION), [node_lib])

    # Upload the fake 64bit node.lib.
    touch_x64_node_lib()
    node_lib = os.path.join(OUT_DIR, 'x64', 'node.lib')
    s3put(bucket, access_key, secret_key, OUT_DIR,
          'atom-shell/dist/{0}'.format(NODE_VERSION), [node_lib])


def update_version(bucket, access_key, secret_key):
  prefix = os.path.join(SOURCE_ROOT, 'dist')
  version = os.path.join(prefix, 'version')
  s3put(bucket, access_key, secret_key, prefix, 'atom-shell', [version])


def s3_config():
  config = (os.environ.get('ATOM_SHELL_S3_BUCKET', ''),
            os.environ.get('ATOM_SHELL_S3_ACCESS_KEY', ''),
            os.environ.get('ATOM_SHELL_S3_SECRET_KEY', ''))
  message = ('Error: Please set the $ATOM_SHELL_S3_BUCKET, '
             '$ATOM_SHELL_S3_ACCESS_KEY, and '
             '$ATOM_SHELL_S3_SECRET_KEY environment variables')
  assert all(len(c) for c in config), message
  return config


def s3put(bucket, access_key, secret_key, prefix, key_prefix, files):
  args = [
    's3put',
    '--bucket', bucket,
    '--access_key', access_key,
    '--secret_key', secret_key,
    '--prefix', prefix,
    '--key_prefix', key_prefix,
    '--grant', 'public-read'
  ] + files

  subprocess.check_call(args)


def touch_x64_node_lib():
  x64_dir = os.path.join(OUT_DIR, 'x64')
  safe_mkdir(x64_dir)
  with open(os.path.join(x64_dir, 'node.lib'), 'w+') as node_lib:
    node_lib.write('Invalid library')


if __name__ == '__main__':
  import sys
  sys.exit(main())
