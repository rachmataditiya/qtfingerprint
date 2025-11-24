pluginManagement {
    repositories {
        google()
        mavenCentral()
        gradlePluginPortal()
    }
}

dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
    repositories {
        google()
        mavenCentral()
    }
}

rootProject.name = "FingerprintPoC"
include(":app")
include(":libdigitalpersona:app")
project(":libdigitalpersona:app").projectDir = File("../libdigitalpersona/app")
