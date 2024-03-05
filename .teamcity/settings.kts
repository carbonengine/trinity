import jetbrains.buildServer.configs.kotlin.*
import jetbrains.buildServer.configs.kotlin.triggers.vcs

/*
The settings script is an entry point for defining a TeamCity
project hierarchy. The script should contain a single call to the
project() function with a Project instance or an init function as
an argument.

VcsRoots, BuildTypes, Templates, and subprojects can be
registered inside the project using the vcsRoot(), buildType(),
template(), and subProject() methods respectively.

To debug settings scripts in command-line, run the

    mvnDebug org.jetbrains.teamcity:teamcity-configs-maven-plugin:generate

command and attach your debugger to the port 8000.

To debug in IntelliJ Idea, open the 'Maven Projects' tool window (View
-> Tool Windows -> Maven Projects), find the generate task node
(Plugins -> teamcity-configs -> teamcity-configs:generate), the
'Debug' option is available in the context menu for the task.
*/

version = "2023.05"

project {

    buildType(DebugWindows)
    buildType(InternalMacOS)
    buildType(InternalWindows)
    buildType(ReleaseMacOS)
    buildType(PublishToPerforce)
    buildType(ReleaseWindows)
    buildType(DebugMacOS)
    buildType(TrinityDevMacOS)
    buildType(TrinityDevWindows)
}

object DebugMacOS : BuildType({
    templates(AbsoluteId("Carbon_GitHubPerforceCMakeProjectTemplate"))
    name = "Debug macOS"

    params {
        param("env.CMAKE_CONFIG_TYPE", "Debug")
        param("env.SENTRY_CLI_DEBUG_SYMBOL_TYPE", "dsym")
    }

    vcs {
        root(DslContext.settingsRoot)
    }

    triggers {
        vcs {
            id = "TRIGGER_193"
            triggerRules = "+:root=${DslContext.settingsRoot.id}:."

        }
    }

    requirements {
        startsWith("teamcity.agent.jvm.os.name", "Mac OS X", "RQ_342")
    }
})

object DebugWindows : BuildType({
    templates(AbsoluteId("Carbon_GitHubPerforceCMakeProjectTemplate"))
    name = "Debug Windows"

    params {
        param("env.CMAKE_CONFIG_TYPE", "Debug")
        param("env.SENTRY_CLI_DEBUG_SYMBOL_TYPE", "pdb")
    }

    vcs {
        root(DslContext.settingsRoot)
    }

    triggers {
        vcs {
            id = "TRIGGER_193"
            triggerRules = "+:root=${DslContext.settingsRoot.id}:."

        }
    }

    requirements {
        startsWith("teamcity.agent.jvm.os.name", "Windows Server 2019", "RQ_342")
    }
})

object InternalMacOS : BuildType({
    templates(AbsoluteId("Carbon_GitHubPerforceCMakeProjectTemplate"))
    name = "Internal macOS"

    params {
        param("env.CMAKE_CONFIG_TYPE", "Internal")
        param("env.SENTRY_CLI_DEBUG_SYMBOL_TYPE", "dsym")
    }

    vcs {
        root(DslContext.settingsRoot)
    }

    triggers {
        vcs {
            id = "TRIGGER_193"
            triggerRules = "+:root=${DslContext.settingsRoot.id}:."

        }
    }

    requirements {
        startsWith("teamcity.agent.jvm.os.name", "Mac OS X", "RQ_342")
    }
})

object InternalWindows : BuildType({
    templates(AbsoluteId("Carbon_GitHubPerforceCMakeProjectTemplate"))
    name = "Internal Windows"

    params {
        param("env.CMAKE_CONFIG_TYPE", "Internal")
        param("env.SENTRY_CLI_DEBUG_SYMBOL_TYPE", "pdb")
    }

    vcs {
        root(DslContext.settingsRoot)
    }

    triggers {
        vcs {
            id = "TRIGGER_193"
            triggerRules = "+:root=${DslContext.settingsRoot.id}:."

        }
    }

    requirements {
        startsWith("teamcity.agent.jvm.os.name", "Windows Server 2019", "RQ_342")
    }
})

object PublishToPerforce : BuildType({
    templates(AbsoluteId("Carbon_PublishToPerforceTemplate"))
    name = "Publish to Perforce"

    enablePersonalBuilds = false
    type = BuildTypeSettings.Type.DEPLOYMENT
    maxRunningBuilds = 1

    params {
        param("perforce_path_to_publish_into", "vendor/github.com/ccpgames/carbon-trinity")
    }

    dependencies {
        dependency(DebugMacOS) {
            snapshot {
            }

            artifacts {
                id = "ARTIFACT_DEPENDENCY_547"
                artifactRules = "**/*=>/%eve_branch_root%/%perforce_path_to_publish_into%/${DebugMacOS.depParamRefs["env.GIT_TAG_HASH"]}"
            }
        }
        dependency(DebugWindows) {
            snapshot {
            }

            artifacts {
                id = "ARTIFACT_DEPENDENCY_549"
                artifactRules = "**/*=>/%eve_branch_root%/%perforce_path_to_publish_into%/${DebugWindows.depParamRefs["env.GIT_TAG_HASH"]}"
            }
        }
        dependency(InternalMacOS) {
            snapshot {
            }

            artifacts {
                id = "ARTIFACT_DEPENDENCY_570"
                artifactRules = "**/*=>/%eve_branch_root%/%perforce_path_to_publish_into%/${InternalMacOS.depParamRefs["env.GIT_TAG_HASH"]}"
            }
        }
        dependency(InternalWindows) {
            snapshot {
            }

            artifacts {
                id = "ARTIFACT_DEPENDENCY_571"
                artifactRules = "**/*=>/%eve_branch_root%/%perforce_path_to_publish_into%/${InternalWindows.depParamRefs["env.GIT_TAG_HASH"]}"
            }
        }
        dependency(ReleaseMacOS) {
            snapshot {
            }

            artifacts {
                id = "ARTIFACT_DEPENDENCY_572"
                artifactRules = "**/*=>/%eve_branch_root%/%perforce_path_to_publish_into%/${ReleaseMacOS.depParamRefs["env.GIT_TAG_HASH"]}"
            }
        }
        dependency(ReleaseWindows) {
            snapshot {
            }

            artifacts {
                id = "ARTIFACT_DEPENDENCY_573"
                artifactRules = "**/*=>/%eve_branch_root%/%perforce_path_to_publish_into%/${ReleaseWindows.depParamRefs["env.GIT_TAG_HASH"]}"
            }
        }
        dependency(TrinityDevMacOS) {
            snapshot {
            }

            artifacts {
                id = "ARTIFACT_DEPENDENCY_574"
                artifactRules = "**/*=>/%eve_branch_root%/%perforce_path_to_publish_into%/${TrinityDevMacOS.depParamRefs["env.GIT_TAG_HASH"]}"
            }
        }
        dependency(TrinityDevWindows) {
            snapshot {
            }

            artifacts {
                id = "ARTIFACT_DEPENDENCY_575"
                artifactRules = "**/*=>/%eve_branch_root%/%perforce_path_to_publish_into%/${TrinityDevWindows.depParamRefs["env.GIT_TAG_HASH"]}"
            }
        }
    }
    
    disableSettings("RUNNER_851")
})

object ReleaseMacOS : BuildType({
    templates(AbsoluteId("Carbon_GitHubPerforceCMakeProjectTemplate"))
    name = "Release macOS"

    params {
        param("env.CMAKE_CONFIG_TYPE", "Release")
        param("env.SENTRY_CLI_DEBUG_SYMBOL_TYPE", "dsym")
    }

    vcs {
        root(DslContext.settingsRoot)
    }

    triggers {
        vcs {
            id = "TRIGGER_193"
            triggerRules = "+:root=${DslContext.settingsRoot.id}:."

        }
    }

    requirements {
        startsWith("teamcity.agent.jvm.os.name", "Mac OS X", "RQ_342")
    }
})

object ReleaseWindows : BuildType({
    templates(AbsoluteId("Carbon_GitHubPerforceCMakeProjectTemplate"))
    name = "Release Windows"

    params {
        param("env.CMAKE_CONFIG_TYPE", "Release")
        param("env.SENTRY_CLI_DEBUG_SYMBOL_TYPE", "pdb")
    }

    vcs {
        root(DslContext.settingsRoot)
    }

    triggers {
        vcs {
            id = "TRIGGER_193"
            triggerRules = "+:root=${DslContext.settingsRoot.id}:."

        }
    }

    requirements {
        startsWith("teamcity.agent.jvm.os.name", "Windows Server 2019", "RQ_342")
    }
})

object TrinityDevMacOS : BuildType({
    templates(AbsoluteId("Carbon_GitHubPerforceCMakeProjectTemplate"))
    name = "TrinityDev macOS"

    params {
        param("env.CMAKE_CONFIG_TYPE", "TrinityDev")
        param("env.SENTRY_CLI_DEBUG_SYMBOL_TYPE", "dsym")
    }

    vcs {
        root(DslContext.settingsRoot)
    }

    triggers {
        vcs {
            id = "TRIGGER_193"
            triggerRules = "+:root=${DslContext.settingsRoot.id}:."

        }
    }

    requirements {
        startsWith("teamcity.agent.jvm.os.name", "Mac OS X", "RQ_342")
    }
})

object TrinityDevWindows : BuildType({
    templates(AbsoluteId("Carbon_GitHubPerforceCMakeProjectTemplate"))
    name = "TrinityDev Windows"

    params {
        param("env.CMAKE_CONFIG_TYPE", "TrinityDev")
        param("env.SENTRY_CLI_DEBUG_SYMBOL_TYPE", "pdb")
    }

    vcs {
        root(DslContext.settingsRoot)
    }

    triggers {
        vcs {
            id = "TRIGGER_193"
            triggerRules = "+:root=${DslContext.settingsRoot.id}:."

        }
    }

    requirements {
        startsWith("teamcity.agent.jvm.os.name", "Windows Server 2019", "RQ_342")
    }
})
