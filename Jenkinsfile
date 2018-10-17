/**
 * Jenkinsfile existing in a project is seen by jenkins and used to build a
 * project. For documentation, see:
 * https://jenkins.io/doc/book/pipeline/jenkinsfile/. For variables that can
 * be used, see
 * https://jenkins.webscalenetworks.com/job/product/job/control/pipeline-syntax/globals.
 */

/*
 * Compute an environment from a set of properties in a map. This is
 * a separate function because it has to be annotated as non-cps in
 * order to be able to use the standard groovy collect method. Not
 * sure why.
 */
@com.cloudbees.groovy.cps.NonCPS
def environment(props) {
  props.collect{ k, v -> k + '=' + v }
}

node {
  /* Checks out the main source tree */
  stage('scm') {
    /* Delete the entire directory first. */
    deleteDir()
    dir('mod_pagespeed') {
      checkout(scm)
    }
  }

  dir('mod_pagespeed') {
    sh('./compute-version ' + env.BRANCH_NAME)
  }

  /* Read the build properties written by compute version. */
  withEnv(environment(readProperties(file: 'build.properties'))) {
    echo('Building ' + env.PRODUCT_VERSION + ' based on ' + env.PAGESPEED_VERSION)
    currentBuild.displayName = env.PRODUCT_VERSION
    currentBuild.description = sh(
      returnStdout: true,
      script: 'cd mod_pagespeed; git log "--pretty=format:%s (%an)" -1'
    )
    stage('build') {
      sh('mod_pagespeed/build.sh ' + env.PRODUCT_VERSION + ' ' + env.BUILDTYPE + ' ' + env.LASTCHANGE)
    }
  }
}
