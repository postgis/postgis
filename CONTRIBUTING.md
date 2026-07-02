# Planning milestones and tracking issues

PostGIS development planning happens via a
[Trac instance](https://trac.osgeo.org/postgis/).

In order for proper scheduling of contributions acceptance/merging it
is recommended to file a ticket there and add your patch or a link
to a patch or to a public git branch where your development is ongoing.

Filing a ticket requires setting up an
[OSGeo account](https://id.osgeo.org),
which lets you contribute to every project of the
[Open Source Geospatial Foundation](https://www.osgeo.org/).

# Getting the code

PostGIS source code is maintained in a git repository hosted
by the Open Source Geospatial Foundation (OSGeo).

You can clone the repository with:

  git clone https://gitea.osgeo.org/postgis/postgis.git
  cd postgis

We occasionally publish commit notes so it is advisable
to configure your clone so it fetches those too:

  git config --add remote.origin.fetch "+refs/notes/*:refs/notes/*"
  git fetch origin
  git notes list # see git notes --help
  git show fba033028 # an example commit containing notes

# Contributing changes

As we understand you might not want to setup a new account, although it
is easier for such contributions to miss our radars unless they _also_
have a matching Trac ticket, we also welcome contributions via:

  - Pull requests on any of the [PostGIS code mirrors](https://trac.osgeo.org/postgis/wiki/CodeMirrors)

  - Patches [by email](https://git-send-email.io)

  - Patches in [the chat room](https://postgis.net/community/chat/)

# Getting more seriously involved

If you intend to be involved for more than an occasional patch we
recommend you to setup an OSGeo account and subscribe to the
[development mailing list](https://lists.osgeo.org/mailman/listinfo/postgis-devel).
