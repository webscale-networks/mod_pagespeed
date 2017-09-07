/**
 * Jenkinsfile existing in a project is seen by jenkins and used to build a
 * project. For documentation, see:
 * https://jenkins.io/doc/book/pipeline/jenkinsfile/. For variables that can
 * be used, see
 * https://jenkins.webscalenetworks.com/job/product/job/control/pipeline-syntax/globals.
 */

node {
  /* Checks out the main source tree */
  stage('scm') {
    /* Delete the entire directory first. */
    deleteDir()
    dir('mod_pagespeed/src') {
      checkout(scm)
    }
  }

  stage('build') {
    sh('mod_pagespeed/src/build.sh')
  }
}
