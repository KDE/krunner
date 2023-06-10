# KRunner

Framework for Plasma runners

## Introduction

The Plasma workspace provides an application called KRunner which, among other
things, allows one to type into a text area which causes various actions and
information that match the text appear as the text is being typed.

One application for this is the universal runner you can launch with ALT-F2.

This functionality is provided via plugins loaded at runtime called "Runners".
These plugins can be used by any application using the Plasma library. The
KRunner framework is used to write these plugins, as explained in
[this tutorial](https://develop.kde.org/docs/plasma/krunner/)

