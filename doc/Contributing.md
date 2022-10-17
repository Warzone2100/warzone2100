<!-- omit in toc -->
# Contributing to Warzone 2100

First off, thanks for taking the time to contribute! ❤️

All types of contributions are encouraged and valued. See the [Table of Contents](#table-of-contents) for different ways to help and details about how this project handles them. Please make sure to read the relevant section before making your contribution. It will make it a lot easier for us maintainers and smooth out the experience for all involved. The community looks forward to your contributions. 🎉

> And if you like the project, but just don't have time to contribute, that's fine. There are other easy ways to support the project and show your appreciation, which we would also be very happy about:
>
> - Star the project
> - Tweet about it
> - Refer this project in your project's readme
> - Mention the project at local meetups and tell your friends/colleagues
> - Post your adventures on Reddit

<!-- omit in toc -->
## Table of Contents

- [I Have a Question](#i-have-a-question)
- [I Want To Contribute](#i-want-to-contribute)
- [Reporting Bugs](#reporting-bugs)
- [Suggesting Enhancements](#suggesting-enhancements)
- [Multiplayer balance changes](#multiplayer-balance-changes)
- [Join The Project Team](#join-the-project-team)

## I Have a Question

> If you want to ask a question, we assume that you have read the available [Documentation](https://github.com/Warzone2100/warzone2100/tree/master/doc).

Before you ask a question, it is best to search for existing [Issues](https://github.com/Warzone2100/warzone2100/issues) that might help you. In case you have found a suitable issue and still need clarification, you can write your question in this issue. It is also advisable to search the internet for answers first.

If you then still feel the need to ask a question and need clarification, we recommend the following:

- Contact and explain your issue to others in the community, maybe they can help you: [Webchat](https://wz2100.net/webchat)
- Open an [Issue](https://github.com/Warzone2100/warzone2100/issues/new).
- Provide as much context as you can about what you're running into.
- Provide game version, mods used and steps to reproduce the issue.

We will then take care of the issue as soon as possible.

## I Want To Contribute

> ### Legal Notice <!-- omit in toc -->
>
> When contributing to this project, you must agree that you have authored 100% of the content, that you have the necessary rights to the content and that the content you contribute may be provided under the project license.

### Reporting Bugs

<!-- omit in toc -->
#### Before Submitting a Bug Report

A good bug report shouldn't leave others needing to chase you up for more information. Therefore, we ask you to investigate carefully, collect information and describe the issue in detail in your report. Please complete the following steps in advance to help us fix any potential bug as fast as possible.

- Make sure that you are using the latest version.
- Determine if your bug is really a bug and not an error on your side e.g. using incompatible environment components/versions (Make sure that you have read the [documentation](https://github.com/Warzone2100/warzone2100/tree/master/doc). If you are looking for support, you might want to check [this section](#i-have-a-question)).
- To see if other users have experienced (and potentially already solved) the same issue you are having, check if there is not already a bug report existing for your bug or error in the [bug tracker](https://github.com/Warzone2100/warzone2100issues?q=label%3Abug).
- Also make sure to search the internet (including Stack Overflow) to see if users outside of the GitHub community have discussed the issue.
- Collect information about the bug:
- Stack trace (Traceback)
- OS, Platform and Version (Windows, Linux, macOS, x86, ARM)
- Version of the interpreter, compiler, SDK, runtime environment, package manager, depending on what seems relevant.
- Possibly your input and the output
- Can you reliably reproduce the issue? And can you also reproduce it with older versions?

#### How Do I Submit a Good Bug Report?

> You must never report security related issues, vulnerabilities or bugs including sensitive information to the issue tracker, or elsewhere in public. Instead sensitive bugs must be sent by email to <>.
> You may add a PGP key to allow the messages to be sent encrypted as well.

We use GitHub issues to track bugs and errors. If you run into an issue with the project:

- Open an [Issue](https://github.com/Warzone2100/warzone2100/issues/new). (Since we can't be sure at this point whether it is a bug or not, we ask you not to talk about a bug yet and not to label the issue.)
- Explain the behavior you would expect and the actual behavior.
- Please provide as much context as possible and describe the *reproduction steps* that someone else can follow to recreate the issue on their own. This usually includes your code. For good bug reports you should isolate the problem and create a reduced test case.
- Provide the information you collected in the previous section.

Once it's filed:

- The project team will label the issue accordingly.
- A team member will try to reproduce the issue with your provided steps. If there are no reproduction steps or no obvious way to reproduce the issue, the team will ask you for those steps.

### Suggesting Enhancements

This section guides you through submitting an enhancement suggestion for Warzone 2100, **including completely new features and minor improvements to existing functionality**. Following these guidelines will help maintainers and the community to understand your suggestion and find related suggestions.

<!-- omit in toc -->
#### Before Submitting an Enhancement

- Make sure that you are using the latest version.
- Read the [documentation](https://github.com/Warzone2100/warzone2100/tree/master/doc) carefully and find out if the functionality is already covered, maybe by an individual configuration.
- Perform a [search](https://github.com/Warzone2100/warzone2100/issues) to see if the enhancement has already been suggested. If it has, add a comment to the existing issue instead of opening a new one.
- Find out whether your idea fits with the scope and aims of the project. It's up to you to make a strong case to convince the project's developers of the merits of this feature. Keep in mind that we want features that will be useful to the majority of our users and not just a small subset. If you're just targeting a minority of users, consider writing a mod.
- Consult [coding style](https://github.com/Warzone2100/warzone2100/blob/master/doc/CodingStyle.md) to avoid conflicting styling of the code.

#### Multiplayer balance changes

Please, keep in mind that some changes can be controversial. Due to multiple low quality changes in multiplayer balance we require in-depth testing of all balance-related changes:

- **Do not pass around your changes**, authoring other people's changes is not allowed. Let everyone else know the hero and eliminate potential telephone game.
- Include **exact parameters** that you changed.
- Thoroughly investigate how proposed changes can impact certain game types and explain your position.
- Attach mod that can be used to **preview changes on unmodified/release version** of the game.
- Playtest thoroughly to **eliminate side effects and publish replay files** that demonstrate gameplay **before and after the change**. (at least 2 before and 2 after, more - better)
- Ask community to leave reviews about proposed change.
- Discuss topic in [discussions](https://github.com/Warzone2100/warzone2100/discussions) section.

If you are not sure about changes that you want to add, don't worry, open an [issue](https://github.com/Warzone2100/warzone2100/issues/new) and let others implement it or suggest compromise/alternative change.

<!-- omit in toc -->
#### How Do I Submit a Good Enhancement Suggestion?

Enhancement suggestions are tracked as [GitHub issues](https://github.com/Warzone2100/warzone2100/issues).

- Use a **clear and descriptive title** for the issue to identify the suggestion.
- Provide a **step-by-step description of the suggested enhancement** in as many details as possible.
- **Describe the current behavior** and **explain which behavior you expected to see instead** and why. At this point you can also tell which alternatives do not work for you.
- You may want to **include screenshots and animated GIFs** which help you demonstrate the steps or point out the part which the suggestion is related to. You can use [this tool](https://www.cockos.com/licecap/) to record GIFs on macOS and Windows, and [this tool](https://github.com/colinkeenan/silentcast) or [this tool](https://github.com/GNOME/byzanz) on Linux.
- **Explain why this enhancement would be useful** to most Warzone 2100 users. You may also want to point out the other projects that solved it better and which could serve as inspiration.

## Join The Project Team

Join our Discord server to find other contributors: [Webchat](https://wz2100.net/webchat)
