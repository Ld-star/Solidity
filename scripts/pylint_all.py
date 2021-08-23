#! /usr/bin/env python3

"""
Runs pylint on all Python files in project directories known to contain Python scripts.
"""

from argparse import ArgumentParser
from os import path, walk, system
from sys import exit as exitwith
from textwrap import dedent

PROJECT_ROOT = path.dirname(path.dirname(path.realpath(__file__)))
PYLINT_RCFILE = f"{PROJECT_ROOT}/scripts/pylintrc"

SGR_INFO = "\033[1;32m"
SGR_CLEAR = "\033[0m"

def pylint_all_filenames(dev_mode, rootdirs):
    """ Performs pylint on all python files within given root directory (recursively).  """
    filenames = []
    for rootdir in rootdirs:
        for rootpath, _, filenames_w in walk(rootdir):
            for filename in filenames_w:
                if filename.endswith('.py'):
                    filenames.append(path.join(rootpath, filename))

    checked_count = 0
    failed = []
    for filename in filenames:
        checked_count += 1
        cmdline = "pylint --rcfile=\"{}\" \"{}\"".format(PYLINT_RCFILE, filename)
        print("{}[{}/{}] Running pylint on file: {}{}".format(SGR_INFO, checked_count, len(filenames),
                                                              filename, SGR_CLEAR))
        exit_code = system(cmdline)
        if exit_code != 0:
            if dev_mode:
                return 1, checked_count
            failed.append(filename)

    return len(failed), len(filenames)


def parse_command_line():
    script_description = dedent("""
        Runs pylint on all Python files in project directories known to contain Python scripts.

        This script is meant to be run from the CI but can also be easily used in the local dev
        environment.
    """)

    parser = ArgumentParser(description=script_description)
    parser.add_argument(
        '-d', '--dev-mode',
        dest='dev_mode',
        default=False,
        action='store_true',
        help="Abort on first error."
    )
    return parser.parse_args()


def main():
    options = parse_command_line()

    rootdirs = [
        f"{PROJECT_ROOT}/docs",
        f"{PROJECT_ROOT}/scripts",
        f"{PROJECT_ROOT}/test",
    ]
    (failed_count, total_count) = pylint_all_filenames(options.dev_mode, rootdirs)

    if failed_count != 0:
        exitwith("pylint failed on {}/{} files.".format(failed_count, total_count))
    else:
        print("Successfully tested {} files.".format(total_count))


if __name__ == "__main__":
    main()
