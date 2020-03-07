# Translating Warzone 2100

- [How do I help translate?](#how-do-i-help-translate)
- [Notes for translators](#notes-for-translators)
- [Notes for proofreaders](#notes-for-proofreaders)
- [Notes for developers](#notes-for-developers)
- [For developers merging translation PRs](#for-developers-merging-translation-prs)

## How do I help translate?
Help us get Warzone 2100 translated into your native language and reach more people.
- Browse over to our Crowdin project: https://crowdin.com/project/warzone2100
- Select the language you’d like to contribute to / review.
   - If the language you want isn’t listed, please open a new issue at: https://github.com/Warzone2100/warzone2100/issues/new
- If you don’t have a Crowdin account yet, you can sign up with an email, or just log-in using another account (GitHub, Google, Twitter, etc).
- Request access to the languages you’d like to help contribute translations to!

## Notes for translators:
- All translation happens in the Crowdin project. Do not open pull requests directly on the warzone2100 repository for translation updates.
   - This helps us ensure consistency of .po file output, provide a measure of Quality Assurance, and keep discussions about translations in a single place where they are in-line with the strings themselves.
- By participating in this project, you agree to the code of conduct: https://github.com/Warzone2100/warzone2100/blob/master/.github/CODE_OF_CONDUCT.md
- If you find an error in the source English strings, open a new issue / pull request on the Warzone2100/warzone2100 repository.
- If you've been working as a translator and want to have more influence over the approved translations in your language, let us know and we'll make you a proofreader.
   - Difference between translators and proofreaders:
      - Translators: can suggest translations for empty or translated strings, and vote on translations
      - Proofreaders: maintain the language - can translate, and approve / reject suggestions
- Discussion about specific translations can occur in-line (see the Comments section to the side, when viewing a translated string).

## Notes for proofreaders:
- Please remember to approve strings where you're satisfied with the translation. This makes it a lot easier to distinguish / review suggested changes from others.

## Notes for developers:
- Source language is English.
- If you add a new source file with translatable strings, be sure to add it to `POTFILES.in`.
   - This will ensure its strings get picked up and added to the po template file.
- If you modify strings in the various `.json` files under `data/`, be sure to run `update-po.sh` to rebuild the extracted json strings.
   > Various scripts called by `update-po.sh` require Python 3.3+
- When you build (with CMake), the CMake build toolchain should automatically update the in-repo po template file (`/po/warzone2100_en.pot`) if needed. Be sure to commit these updates with your related changes!

## For developers merging translation PRs:
- When **merging** a "**New Crowdin translations**" PR:
   - Make sure the CI passes succeeded. If there’s an issue with string formatting / output, the CI passes may / should fail. Verify the CI logs on failures - if they show an issue with translations, _do **NOT** merge the PR_.
   - You may want to use "**Squash and Merge**" if there’s a gigantic list of commits (or repeated updates of the same language files) to minimize clutter in the main commit history.
   - **Delete the `l10n_master` branch after the PR is merged**
      - This ensures that Crowdin starts fresh when it next pushes a translation changes PR, based on the latest master.
- If a "**New Crowdin translations**" PR can’t be merged (ex. because of merge conflicts / outdated translation branch):
   - Close the Pull Request.
   - Delete the `l10n_master` branch.
   > Crowdin will then (eventually) re-create the `l10n_master` branch, and submit a new PR, effectively rebased on the latest master branch. _This may take up to 24 hours to occur, as we have configured Crowdin to batch pushes to its `l10n_*` service branch._
   
