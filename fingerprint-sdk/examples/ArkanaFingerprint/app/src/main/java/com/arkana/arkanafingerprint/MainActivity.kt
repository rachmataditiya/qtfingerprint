package com.arkana.arkanafingerprint

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.ui.Modifier
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import com.arkana.arkanafingerprint.ui.screen.HomeScreen
import com.arkana.arkanafingerprint.ui.screen.EnrollScreen
import com.arkana.arkanafingerprint.ui.screen.VerifyScreen
import com.arkana.arkanafingerprint.ui.screen.IdentifyScreen
import com.arkana.arkanafingerprint.ui.screen.SettingsScreen
import com.arkana.arkanafingerprint.ui.screen.UserListScreen
import com.arkana.arkanafingerprint.ui.screen.CreateUserScreen
import com.arkana.arkanafingerprint.ui.theme.ArkanaFingerprintTheme

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            ArkanaFingerprintTheme {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    val navController = rememberNavController()
                    NavHost(
                        navController = navController,
                        startDestination = "home"
                    ) {
                        composable("home") {
                            HomeScreen(
                                onNavigateToEnroll = { navController.navigate("enroll") },
                                onNavigateToVerify = { navController.navigate("verify") },
                                onNavigateToIdentify = { navController.navigate("identify") },
                                onNavigateToSettings = { navController.navigate("settings") },
                                onNavigateToUsers = { navController.navigate("users") }
                            )
                        }
                        composable("enroll") {
                            EnrollScreen(
                                onNavigateBack = { navController.popBackStack() },
                                onNavigateToUsers = { navController.navigate("users") }
                            )
                        }
                        composable("users") {
                            UserListScreen(
                                onNavigateBack = { navController.popBackStack() },
                                onUserSelected = { user ->
                                    // Navigate back to enroll with selected user
                                    navController.popBackStack()
                                },
                                onCreateUser = { navController.navigate("create_user") }
                            )
                        }
                        composable("create_user") {
                            CreateUserScreen(
                                onNavigateBack = { navController.popBackStack() },
                                onUserCreated = { navController.popBackStack() }
                            )
                        }
                        composable("verify") {
                            VerifyScreen(
                                onNavigateBack = { navController.popBackStack() },
                                onNavigateToUsers = { navController.navigate("users") }
                            )
                        }
                        composable("identify") {
                            IdentifyScreen(
                                onNavigateBack = { navController.popBackStack() }
                            )
                        }
                        composable("settings") {
                            SettingsScreen(
                                onNavigateBack = { navController.popBackStack() }
                            )
                        }
                    }
                }
            }
        }
    }
}
