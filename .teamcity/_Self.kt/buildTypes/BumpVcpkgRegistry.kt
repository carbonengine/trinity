// Copyright © 2026 CCP ehf.

package _Self.buildTypes

import jetbrains.buildServer.configs.kotlin.AbsoluteId
import jetbrains.buildServer.configs.kotlin.BuildType
import jetbrains.buildServer.configs.kotlin.CheckoutMode
import jetbrains.buildServer.configs.kotlin.DslContext
import jetbrains.buildServer.configs.kotlin.ParameterDisplay
import jetbrains.buildServer.configs.kotlin.buildFeatures.sshAgent
import jetbrains.buildServer.configs.kotlin.buildSteps.ScriptBuildStep
import jetbrains.buildServer.configs.kotlin.buildSteps.script
import jetbrains.buildServer.configs.kotlin.triggers.vcs

object BumpVcpkgRegistry : BuildType({
    name = "Bump vcpkg Registry"
    description = "Opens a draft PR in the vcpkg registry that bumps this component's port when a vMAJOR.MINOR.PATCH tag is created"

    enablePersonalBuilds = false
    maxRunningBuilds = 1

    params {
        param("vcpkg_port_name", "carbon-trinity")
        param("github_registry_repository", "carbonengine/vcpkg-registry")
        param("git_user_name", "Carbon Automation")
        param("git_user_email", "213465544+ccpgames-carbon@users.noreply.github.com")
        param("env.GITHUB_TOKEN", "%GITHUB_CARBON_PAT%")
        text("env.VCPKG_MANIFEST_ONLY_DEPENDENCIES", "carbon-exefile", label = "Manifest only dependencies", display = ParameterDisplay.NORMAL, allowEmpty = true,
            description = "Comma separated dependencies that belong in this component's vcpkg.json but not in its registry port, e.g. dependencies only used by its tests or tooling. Any other dependency missing from the port is added to it.")
        param("teamcity.vcsTrigger.runBuildInNewEmptyBranch", "true")
    }

    vcs {
        root(DslContext.settingsRootId, "+:. => component")
        root(AbsoluteId("CarbonVcpkgRegistry"), "+:. => registry")
        root(AbsoluteId("CarbonPipelineTools"), "+:. => carbon_pipeline_tools")
        checkoutMode = CheckoutMode.ON_AGENT
        cleanCheckout = true
    }

    steps {
        script {
            name = "Open registry PR for release tag"
            scriptContent = """
                set -euo pipefail

                apt-get update -qq && apt-get install -y -qq --no-install-recommends zip
                export GIT_SSH_COMMAND="ssh -o StrictHostKeyChecking=accept-new"
                git config --global --add safe.directory '*'
                git config --global user.name "%git_user_name%"
                git config --global user.email "%git_user_email%"

                python3 carbon_pipeline_tools/carbon/bump-vcpkg-registry.py \
                    --tag "%teamcity.build.vcs.branch.${DslContext.settingsRootId.id}%" \
                    --commit "%build.vcs.number.${DslContext.settingsRootId.id}%" \
                    --port "%vcpkg_port_name%" \
                    --component-dir component \
                    --registry-dir registry \
                    --registry-repository "%github_registry_repository%" \
                    --vcpkg-root component/vendor/github.com/microsoft/vcpkg
            """.trimIndent()
            dockerImage = "python:3.12"
            dockerImagePlatform = ScriptBuildStep.ImagePlatform.Linux
        }
    }

    triggers {
        vcs {
            triggerRules = "+:root=${DslContext.settingsRootId.id}:."
            // Tags are reported as logical branch names (e.g. v3.2.0), which requires "Use tags as branches" on the VCS root
            branchFilter = """
                +:v*
            """.trimIndent()
        }
    }

    features {
        sshAgent {
            teamcitySshKey = "ccpgames-carbon"
        }
    }

    requirements {
        contains("teamcity.agent.jvm.os.name", "Linux")
        // vcpkg only publishes a prebuilt Linux binary for x64, elsewhere bootstrapping it builds from source
        equals("teamcity.agent.jvm.os.arch", "amd64")
    }
})
