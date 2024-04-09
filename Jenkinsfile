pipeline {
  //agent to be built on
  agent {
    label 'hawkeye-app'
  }
  
  //cron trigger for nightly build
  triggers{
    cron(env.BRANCH_NAME == 'main' || env.BRANCH_NAME == 'master' ? 'H H(4-10) * * 1-5' : '')
  }

  //global variables
  environment {
    //Version of Maven to use
    MavenVersion = 'Maven 3.8.3'
    //list of email recipients when main fails
    NightlyEmailList = 'tsteinbaugh@beckman.com, pmills@beckman.com'
    ReleaseEmailList = "${env.NightlyEmailList}, mbevers@beckman.com, nkapoor@beckman.com"
  } 

  options {
    //skips default SCM checkout.  This is needed in order to checkout with SSH url
    skipDefaultCheckout(true)

    ///build notifications; being sent to MS Teams
    office365ConnectorWebhooks([
      [name: "Office 365", 
       url: "https://danaher.webhook.office.com/webhookb2/1df099f8-a4c0-4fe6-8a47-725c35e271fe@771c9c47-7f24-44dc-958e-34f8713a8394/IncomingWebhook/df119c4e42d6431fabd8226cb6e0b22d/3e449ca1-6c1d-426b-a458-3b5d029e2670",
       notifyBackToNormal: true, 
       notifyFailure: true, 
       notifyRepeatedFailure: true, 
       notifySuccess: true, 
       notifyAborted: true]
      ])    

    //number of builds to save in history
    buildDiscarder logRotator(
      daysToKeepStr: '14', 
      numToKeepStr: '9')
  }
  
  //boolean parameter for release build; allows user to go into Jenkins and request a release build via the "Build with Parameters" option
  parameters {
     booleanParam(
       name: 'Release Build', 
       defaultValue: false, 
       description: 'Release Build Indicator')
  }
      
  stages{
    //Nightly build
    stage ('Nightly build') {
      when {
        triggeredBy 'TimerTrigger'
        anyOf{
          branch "main" 
          branch "master"
        }
      }
      stages {
        stage('Nigthly build - build'){
          steps {
            cleanWs()
            checkout([
              $class: 'GitSCM', 
              branches: [[name: '*/master']], 
              extensions: [[$class: 'LocalBranch', localBranch: 'master']], 
              userRemoteConfigs: [[credentialsId: '30bccc90-3e99-4958-b9a7-e822c26d0cdf', url: 'git@github.com:BeclsParticle/sciencemodule_ApplicationSource.git']]
            ])
            withMaven (maven: env.MavenVersion){
              sh "mvn -U clean deploy site site:deploy"
            }
          }
        }
      }
      //send email for  nightly build
      post {
        failure {
          emailext (
            subject: '$JOB_NAME failed',
            body: '$JOB_NAME, job $BUILD_ID failed the nightly build on $BUILD_TIMESTAMP.\n$BUILD_URL',
            to: env.NightlyEmailList)
        }
        success {
          //trigger gRpc job when current job is successful
          build (
            job: 'Shepherd/ViabilityScienceModule-gRpc/master', 
            propagate: false, 
            wait: false,
            parameters: [[
              $class: 'StringParameterValue',
              name: 'triggerType',
              value: 'nightly']])
        }
      }
    }
    //build for feature branches
    stage ('Feature branch testing') {
      when {
        //a feature branch must follow <initials>-<JIRA#>-<description> e.g. xyz-1234-foobar
        branch pattern: "\\w+-\\d+-\\S*", comparator: "REGEXP"
        expression {params.'Release Build' == false}
      }
      steps {
        cleanWs()
        checkout([
          $class: 'GitSCM', 
          branches: [[name: "refs/heads/${env.BRANCH_NAME}"]], 
          extensions: [[$class: 'LocalBranch']], 
          userRemoteConfigs: [[credentialsId: '30bccc90-3e99-4958-b9a7-e822c26d0cdf', url: 'git@github.com:BeclsParticle/sciencemodule_ApplicationSource.git']]
        ])
        withMaven (maven: env.MavenVersion){
          sh 'mvn -U clean deploy'
        }
      }
      post {
        success {
          script {
            try {
              build (
                job: "Shepherd/ViabilityScienceModule-gRpc/${env.BRANCH_NAME}", 
                propagate: false, 
                wait: false,
                parameters: [
                  [$class: 'StringParameterValue',
                  name: 'triggerType',
                  value: 'feature']
                ])
            }
            catch (err) {
              build (
                job: "Shepherd/ViabilityScienceModule-gRpc/master", 
                propagate: false, 
                wait: false,
                parameters: [
                  [$class: 'StringParameterValue',
                  name: 'triggerType',
                  value: 'feature']
                ])
            }
          }
        }
      }
    }
    //build for PR with the main/master branch as target
    stage ('Pull request for Origin (Main/Master)') {
      when {
        branch "PR-*"
        expression {return changeRequest (target: "master") || changeRequest (target: "main")}
        expression {params.'Release Build' == false}
      }
      steps {
        cleanWs()
        checkout scm
        withMaven (maven: env.MavenVersion){
          sh 'mvn -U clean package'
        }
      }
    }
    //Release build; triggered by the boolean parameter; updates the pom file to next revision (when it builds succesfully)
    stage('Release build'){
      when{
        expression {params.'Release Build' == true}
        anyOf {
          branch 'main'
          branch 'master'
        }
      }
      stages {
        stage ('Release build - remove "SNAPSHOT" from pom version') {
          steps {
            cleanWs()
            checkout([
              $class: 'GitSCM', 
              branches: [[name: '*/master']], 
              extensions: [[$class: 'LocalBranch', localBranch: 'master']], 
              userRemoteConfigs: [[credentialsId: '30bccc90-3e99-4958-b9a7-e822c26d0cdf', url: 'git@github.com:BeclsParticle/sciencemodule_ApplicationSource.git']]
            ])
            script {
                def mavenPom = readMavenPom file: 'pom.xml'
                echo "original pom version: ${mavenPom.version}"
            }
            withMaven (maven: env.MavenVersion){
              sh 'mvn build-helper:parse-version versions:set -DnewVersion=\\${parsedVersion.majorVersion}.\\${parsedVersion.minorVersion}.\\${parsedVersion.incrementalVersion}'
            }
            script {
                def mavenPom = readMavenPom file: 'pom.xml'
                echo "modified pom version: ${mavenPom.version}"
            }
          }
        }
        stage('Release build - build'){
          steps {
            withMaven (maven: env.MavenVersion){
              sh "mvn -U clean deploy site site:deploy scm:tag"
            }
          }
        }
        stage('Release build - checkout'){
          steps {
            withMaven (maven: env.MavenVersion){
              sh "git checkout -f master"
            }
          }
        }
        stage('Release build - next pom version and add "SNAPSHOT"'){
          steps {
            script {
              def mavenPom = readMavenPom file: 'pom.xml'
              echo "original pom version: ${mavenPom.version}"
            }
            withMaven (maven: env.MavenVersion){
              sh 'mvn build-helper:parse-version versions:set -DnewVersion=\\${parsedVersion.majorVersion}.\\${parsedVersion.minorVersion}.\\${parsedVersion.nextIncrementalVersion}-SNAPSHOT'
            }
            script {
              def mavenPom = readMavenPom file: 'pom.xml'
              echo "modified pom version: ${mavenPom.version}"
            }
          }
        }
        stage('Release build - commit new pom version to GitHub'){
          steps {
            withMaven (maven: env.MavenVersion){
              sh 'mvn versions:commit scm:checkin -Dmessage=testmessage -DpushChanges=true'
            }
            script {
              def mavenPom = readMavenPom file: 'pom.xml'
              echo "committed pom version: ${mavenPom.version}"
            }
          }
        }
      }
      //send email notification for release build
      post {
        failure {
          emailext (
            subject: 'Release $JOB_NAME failed',
            body: 'Release $JOB_NAME, job $BUILD_ID failed the release build on $BUILD_TIMESTAMP.\n$BUILD_URL',
            to: env.ReleaseEmailList)
        }
        success {
          //creation of URL link to S3 .zip artifact.
          script {
            def S3_url = manager.getLogMatcher ("(.*)Uploaded(.*)+s3(.*)+zip(.*)")
            if (S3_url.matches()) {
              S3_url = S3_url.group(0)
              while(!S3_url.startsWith("snapshot") && !S3_url.startsWith("release"))
                S3_url = S3_url.substring(1)
              while (!S3_url.endsWith(".zip"))
                S3_url = S3_url.substring(0, S3_url.length() - 1)
              S3_url = "http://beckman-build-artifacts.s3-us-west-2.amazonaws.com/m2/" + S3_url
            }
            def site_url = manager.getLogMatcher ("(.*)s3://beckman-build-artifacts/docs/(.*)")
            if (site_url.matches()) {
              String site_url2 = site_url.group(0)
              echo site_url2
              while(!site_url2.matches("^com\\.beckman\\.particle\\.(.+)"))
                site_url2 = site_url2.substring(1)
              
              while (!site_url2.matches('SNAPSHOT$') && (!site_url2.matches('.+\\d+\\.\\d+\\.\\d+$')))
                site_url2 = site_url2.substring(0, site_url2.length() - 1)
              
              site_url = "https://beckman-build-artifacts.s3.us-west-2.amazonaws.com/docs/" + site_url2 + "/index.html"
            }
            emailext (
              subject: '$JOB_NAME',
              body: "${JOB_NAME}, job ${BUILD_ID} built successfully on ${BUILD_TIMESTAMP}.\n${BUILD_URL}\nArtifact URL: ${S3_url}\nSite URL: ${site_url}",
              to: env.ReleaseEmailList)
          }
          //trigger gRpc job when current job is successful
          build (
            job: "Shepherd/ViabilityScienceModule-gRpc/master", 
            propagate: false, 
            wait: false,
            parameters: [[
              $class: 'StringParameterValue',
              name: 'triggerType',
              value: 'release']])
        }
      }
    }
    //build for any pushes to main, except nightly build and forecd released build
    stage ('Main branch build (includes merge, does not include nightly or forced relase build') {
      when {
        expression {params.'Release Build' == false}
        anyOf {
          branch 'main'
          branch 'master'
        }
        not {
          triggeredBy 'TimerTrigger'
        }
      }
      steps {
       cleanWs()
       checkout([
          $class: 'GitSCM', 
          branches: [[name: '*/master']], 
          extensions: [[$class: 'LocalBranch', localBranch: 'master']], 
          userRemoteConfigs: [[credentialsId: '30bccc90-3e99-4958-b9a7-e822c26d0cdf', url: 'git@github.com:BeclsParticle/sciencemodule_ApplicationSource.git']]
        ])
        withMaven (maven: env.MavenVersion){
          sh 'mvn -U clean deploy site site:deploy'
        }
      }
      post {
        success {
          //trigger gRpc job when current job is successful
          build (
            job: "Shepherd/ViabilityScienceModule-gRpc/${env.BRANCH_NAME}",
            propagate: false, 
            wait: false,
            parameters: [[
              $class: 'StringParameterValue',
              name: 'triggerType',
              value: 'main']])
        }
      }
    }
    //Stage to check naming convention of branch; if branch is not "main"/"master", feature, nightly build or a PR, the build will fail.
    //This was added to ensure no false passes would occur (i.e. an inapproriately named branch skips all stages and "passes")
    stage('Verify branch syntax') {
      when {
        not {
          anyOf {
            branch 'main'
            branch 'master'
          }
        }
        not {
          branch pattern: "\\w+-\\d+-\\S*", comparator: "REGEXP"
        }
        not {
          branch "PR-*"
       }
        not {
          triggeredBy 'TimerTrigger'
        }
      }
      steps {
        error "invalid branch name - expected <initials>-<JIRA#>-<description>  e.g xyz-1234-foobar  Note: Description should not contain spaces."
      }
    }
  }
}
