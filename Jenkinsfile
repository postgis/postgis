pipeline {
    agent   {
        label {
            image 'docker.osgeo.org/postgis/build-test:trisquel2'
            label 'docker'
            registryUrl 'https://docker.osgeo.org/'
        }
    }

    stages {
        stage('Build') {
            steps {
                echo 'Building..'
            }
        }
        stage('Test') {
            steps {
                echo 'Testing..'
            }
        }
        stage('Deploy') {
            steps {
                echo 'Deploying....'
            }
        }
    }
}
