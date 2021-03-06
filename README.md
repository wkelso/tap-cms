TAP
===

TAP aims to provide tools to easily create and deliver mobile tour applications in a museum setting. Content creation is performed in the content management system, Drupal. TAP tours are exportable into an intermediate format, TourML, which can then be used as pluggable bundles for mobile applications.

License
-------
TAP is released under the GPLv3 license.  See GPL.txt for more information

Installation
------------

### Install via Drush (Recommended)

Installing TAP via drush will download drupal, all contributed modules, and the TAP module.

NOTE: To install TAP via this method you must be running the latest 5.x branch of drush.

    # pear channel-discover pear.drush.org
    # pear install drush/drush-5.0.0

Once you have drush installed you will need to download and process the drush make file to proceed.

    # wget https://raw.github.com/IMAmuseum/tap-cms/master/drush.make
    # drush make drush.make

Drush will attempt to fetch Drupal and all related modules and libraries. Now that you have all of the files required to setup TAP, Follow these instructions to finish the installation of Drupal.

Once Drupal is installed and configured visit http://mytapsite.com/admin/modules. Find the module labeled "Tap" and enable it. Congratulations, you have successfully install the Tap Content Management System.

### Install the TAP Drupal module

If you already have an existing drupal site that you would like to install TAP on follow the directions below.

    # cd /PATH_TO_DRUPAL_SITE/sites/all/modules
    # wget https://github.com/IMAmuseum/tap-cms/tarball/v2.0-alpha1
    # tar zxvf v2.0-alpha1

Once you have unpackaged the TAP CMS visit http://mysite.com/admin/modules and enable the TAP module.
