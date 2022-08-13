pipeline {
    agent   {
        label {
            label 'jarmmie-drone'
        }
    }

    stages {
        stage('Build') {
            steps {
              sh './ci/debbie-pipeline/postgis_build.sh'
            }
        }
        stage('Test') {
            steps {
                sh './ci/debbie-pipeline/postgis_regress.sh'
            }
        }
    }
}
