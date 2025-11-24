plugins {
    id("com.android.library")
    id("org.jetbrains.kotlin.android")
}

android {
    namespace = "com.arkana.libdigitalpersona"
    compileSdk = 34

    defaultConfig {
        minSdk = 29
        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
        ndk {
            abiFilters += listOf("arm64-v8a") // Only build for arm64-v8a (libfprint built for this arch)
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }
    kotlinOptions {
        jvmTarget = "11"
    }
    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
    buildFeatures {
        viewBinding = true
    }
    
    // Copy libfprint and dependencies to jniLibs
    val ndkPrefix = file("${project.rootDir}/../../android-ndk-prefix")
    if (ndkPrefix.exists()) {
        val jniLibsDir = file("src/main/jniLibs")
        jniLibsDir.mkdirs()
        
        val archDirs = listOf("arm64-v8a", "armeabi-v7a", "x86_64")
        archDirs.forEach { arch ->
            val archDir = file("$jniLibsDir/$arch")
            archDir.mkdirs()
            
            // Copy all .so files from android-ndk-prefix/lib
            val libDir = file("$ndkPrefix/lib")
            if (libDir.exists()) {
                libDir.listFiles()?.filter { it.name.endsWith(".so") }?.forEach { libFile ->
                    val destFile = file("$archDir/${libFile.name}")
                    if (!destFile.exists() || destFile.lastModified() < libFile.lastModified()) {
                        libFile.copyTo(destFile, overwrite = true)
                        println("Copied ${libFile.name} to $archDir")
                    }
                }
            }
        }
    }
}

dependencies {
    implementation(libs.androidx.core.ktx)
    implementation(libs.androidx.appcompat)
    implementation(libs.material)
    implementation(libs.androidx.constraintlayout)
    implementation(libs.androidx.biometric)
    implementation(libs.androidx.fragment)
    testImplementation(libs.junit)
    androidTestImplementation(libs.androidx.junit)
    androidTestImplementation(libs.androidx.espresso.core)
}