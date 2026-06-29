// Copyright © 2026 CCP ehf.

package _Self.buildTypes

import jetbrains.buildServer.configs.kotlin.*
import jetbrains.buildServer.configs.kotlin.buildFeatures.vcsLabeling
import jetbrains.buildServer.configs.kotlin.triggers.vcs
import jetbrains.buildServer.configs.kotlin.buildSteps.python
import jetbrains.buildServer.configs.kotlin.buildSteps.powerShell



class Publish(perforce_publish_path: String) : BuildType({
    name = "Publish to Perforce"

    enablePersonalBuilds = false
    type = BuildTypeSettings.Type.DEPLOYMENT
    maxRunningBuilds = 1

    params {
        select("project", "frontier", label = "Which project to publish into?", description = "e.g. EVE or Frontier or Frontier Stream", display = ParameterDisplay.PROMPT,
                options = listOf("EVE Online" to "eve", "Frontier" to "eve-frontier", "Frontier Stream" to "frontier"))
        
        param("perforce_path_to_publish_into", perforce_publish_path)
        
        param("env.P4PASSWD", "%platform_automation_pass%")
        param("env.P4USER", "platform_automation")
        text("reverse.dep.*.env.GIT_TAG_HASH_OVERRIDE", "", label = "GTH override for component version in vendor", description = "GIT TAG HASH OVERRIDE, IF NEEDED FOR DEPENDENCIES", display = ParameterDisplay.PROMPT, allowEmpty = true)
        param("env.TC_BUILD_URL", "%teamcity.serverUrl%/viewLog.html?buildId=%teamcity.build.id%")
        text("eve_branch_shortname", "", label = "Branch Name", description = """The name of the branch, for example MAINLINE""", display = ParameterDisplay.PROMPT, allowEmpty = false)
        param("env.TC_BUILDID", "%teamcity.build.id%")
        param("env.TC_BUILD_NUMBER", "Carbon Trinity #%build.number%")
        param("env.P4PORT", "p4is.ccp.ad.local:1666")
        param("env.TC_EVE_BRANCH_SHORTNAME", "%eve_branch_shortname%")
        param("env.TC_EVE_PROJECT", "%project%")
        param("carbon-pipeline-tools-ref", "refs/heads/main")
        select("eve_branch_path", "//%project%/%eve_branch_shortname%/", label = "Perforce branch", description = "Is this a regular perforce branch or a perforce stream? Used for setting the proper branch path.", display = ParameterDisplay.PROMPT,
                options = listOf("Regular" to "//%project%/branches/%eve_branch_type%/%eve_branch_shortname%/", "Frontier Stream" to "//%project%/%eve_branch_shortname%/"))
        select("eve_branch_type", "sandbox", label = "Branch type", description = "The type of branch to publish into", display = ParameterDisplay.PROMPT,
                options = listOf("Sandbox" to "sandbox", "Development" to "development", "Release" to "release", "Staging" to "staging", "Stream" to ""))
        param("env.TC_PERFORCE_PATH_TO_PUBLISH_INTO", "%perforce_path_to_publish_into%")
        param("env.EXECUTABLE_FILENAMES_MATCH", "ShaderCompiler")

        select("reverse.dep.*.env.VISUAL_STUDIO_PLATFORM_TOOLSET", "v141", label = "Visual Studio Platform Toolset", description = "Specify the toolset for the build. e.g. v141 or v143.", display = ParameterDisplay.PROMPT,
                options = listOf("v141 (2017)" to "v141", "v143 (2022)" to "v143"))
        text("reverse.dep.*.carbon_ref", "", label = "Ref  Carbon Component", description = "REF for carbon component e.g. refs/heads/main or refs/tags/v1.0.0 or refs/heads/frontier", display = ParameterDisplay.PROMPT, allowEmpty = true)
    }

    vcs {
        root(AbsoluteId("CarbonPipelineTools"), "+:carbon/.=>carbon")

        checkoutMode = CheckoutMode.ON_AGENT
        cleanCheckout = true
    }

    steps {
        python {
            name = "Check builds have matching git tags"
            id = "Check build tags"
            command = script {
                content = """
                    tags = set([
                        "${MacOS.arm64_Release.depParamRefs["env.GIT_TAG_HASH"]}",
                        "${MacOS.arm64_Debug.depParamRefs["env.GIT_TAG_HASH"]}",
                        "${MacOS.arm64_Internal.depParamRefs["env.GIT_TAG_HASH"]}",
                        "${MacOS.arm64_TrinityDev.depParamRefs["env.GIT_TAG_HASH"]}",
                        "${MacOS.x64_Release.depParamRefs["env.GIT_TAG_HASH"]}",
                        "${MacOS.x64_Debug.depParamRefs["env.GIT_TAG_HASH"]}",
                        "${MacOS.x64_Internal.depParamRefs["env.GIT_TAG_HASH"]}",
                        "${MacOS.x64_TrinityDev.depParamRefs["env.GIT_TAG_HASH"]}",
                        "${Windows.Release.depParamRefs["env.GIT_TAG_HASH"]}",
                        "${Windows.Debug.depParamRefs["env.GIT_TAG_HASH"]}",
                        "${Windows.Internal.depParamRefs["env.GIT_TAG_HASH"]}",
                        "${Windows.TrinityDev.depParamRefs["env.GIT_TAG_HASH"]}"
                    ])
                    if len(tags) > 1:
                        raise ValueError(f"Multiple different build tags have been detected {tags}")
                """.trimIndent()
            }
        }
        python {
            name = "Check Publish Branch"
            id = "nopublish_check"
            command = script {
                content = """
                    if '%eve_branch_path%' == '//eve/branches/development/mainline/':
                        print("##teamcity[message text='Check failed: You cannot publish directly to EVE MAINLINE' status='ERROR']")
                        raise ValueError("Check failed: The variable is equal to 'EVE MAINLINE'")
                    else:
                        print("##teamcity[message text='Not publishing to EVE MAINLINE' status='NORMAL']")
                """.trimIndent()
            }
        }
        powerShell {
            name = "Sync perforce"
            id = "Sync_perforce"
            scriptMode = script {
                content = """
                
                    Write-Host "##teamcity[message text='Using %env.P4USER% credentials for project: %project%' status='NORMAL']"
                    Write-Host "Logging in to p4 with user: %env.P4USER% ..."
                    echo %env.P4PASSWD%|p4 -p %env.P4PORT% -u %env.P4USER% login

                    ${'$'}loginResult = p4 login -s 2>&1
                    ${'$'}loginStatus = if (${'$'}LASTEXITCODE -eq 0) { "Valid" } else { "Invalid/Expired" }
                    Write-Host "##teamcity[message text='P4 Login Status: ${'$'}loginStatus (${'$'}loginResult)']"

                    ${'$'}clientName = "TC_p4_%teamcity.agent.name%_%teamcity.build.default.checkoutDir%_%eve_branch_shortname%"
                    Write-Host "Switching to client ${'$'}clientName ..."
                    p4 set P4CLIENT=${'$'}clientName

                    ${'$'}paths = @("TeamCity/Publishers/...", "vendor/python/...", "packages/...", "carbon/common/stdlib/...")
                    ${'$'}syncPaths = @()

                    if ("%project%" -eq "frontier" ) {
                        ${'$'}branchPath = "//%project%/%eve_branch_shortname%"
                        ${'$'}publishPath = "${'$'}branchPath/%perforce_path_to_publish_into%/..."

                        foreach (${'$'}path in ${'$'}paths) {
                            ${'$'}syncPaths += "${'$'}branchPath/${'$'}path"
                        }

                        ${'$'}p4SpecOutput = p4 client -S ${'$'}branchPath -o 2>&1

                        if (${'$'}p4SpecOutput -match "already exists") {
                            Write-Host "Client already exists. Switching stream..."
                            p4 client -t ${'$'}clientName -s -S ${'$'}branchPath
                        }
                        else {
                            Write-Host "Creating/updating client..."
                            ${'$'}p4SpecOutput | p4 client -i
                        }

                        # Force syncing files from Perforce
                        Write-Host "Syncing from perforce with paths: ${'$'}(${'$'}syncPaths -join ' ')"
                        p4 -s sync -q -f ${'$'}syncPaths
                        p4 -s sync -q ${'$'}publishPath
                        p4 -s sync -f -k %teamcity.build.checkoutDir%/%perforce_path_to_publish_into%/...
                    }
                    else {
                        p4 info > p4_info.txt
                        ${'$'}workspaceName = (Select-String p4_info.txt -Pattern "Client name: (.*)").Matches[0].Groups[1].Value
                        Remove-Item p4_info.txt
                        Write-Host "Setting up client ${'$'}workspaceName"

                        ${'$'}branchPath = "//%project%/branches/%eve_branch_type%/%eve_branch_shortname%"
                        ${'$'}workDir = "%teamcity.build.checkoutDir%"
                        ${'$'}publishPath = "${'$'}branchPath/%perforce_path_to_publish_into%/..."

                        foreach (${'$'}path in ${'$'}paths) {
                            ${'$'}syncPaths += "${'$'}branchPath/${'$'}path"
                        }

                        ${'$'}mappings = foreach (${'$'}path in ${'$'}paths) {
                            "${'$'}(${'$'}branchPath)/${'$'}path //${'$'}(${'$'}workspaceName)/${'$'}path"
                        }
                        Write-Host "Mapping: ${'$'}mappings"
                        ${'$'}publishMapping = "${'$'}(${'$'}branchPath)/%perforce_path_to_publish_into%/... //${'$'}(${'$'}workspaceName)/%perforce_path_to_publish_into%/..."
                        Write-Host "Extramapping: ${'$'}publishMapping"


                        p4 --field "View=${'$'}(${'$'}mappings[0])" --field "View+=${'$'}(${'$'}mappings[1])" --field "View+=${'$'}(${'$'}mappings[2])" --field "View+=${'$'}(${'$'}mappings[3])" --field "View+=${'$'}publishMapping" client -o | p4 client -i

                        # Force syncing files from Perforce
                        Write-Host "Syncing from perforce"
                        p4 -s sync -q -f ${'$'}syncPaths
                        p4 -s sync -q ${'$'}publishPath
                        p4 -s sync -f -k %teamcity.build.checkoutDir%/%perforce_path_to_publish_into%/...
                    }
                """.trimIndent()
            }
        }
        python {
            name = "Publish-to-Perforce"
            environment = venv {
                requirementsFile = ""
            }
            command = file {
                filename = "carbon/publish-to-perforce.py"
                scriptArguments = "%system.teamcity.build.workingDir%"
            }
        }
        powerShell {
            name = "Python-to-Perforce"
            enabled = false
            formatStderrAsError = true
            scriptMode = file {
                path = "TeamCity/Publishers/Python-To-Perforce.ps1"
            }
            scriptArgs = "%perforce_path_to_publish_into% packages"
        }
        powerShell {
            name = "Delete p4 workspace"
            id = "Delete_p4_workspace"
            executionMode = BuildStep.ExecutionMode.ALWAYS
            scriptMode = script {
                content = """
                    ${'$'}clientName = "TC_p4_%teamcity.agent.name%_%teamcity.build.default.checkoutDir%_%eve_branch_shortname%"
                    Write-Host "Deleting workspace ${'$'}clientName"
                    p4 client -d ${'$'}clientName
                """.trimIndent()
            }
        }
    }

    dependencies {
        dependency(CreateUniversalBuilds) {
            snapshot {
                onDependencyFailure = FailureAction.FAIL_TO_START
            }

            artifacts {
                artifactRules = "artifact.zip!**=>%perforce_path_to_publish_into%/${MacOS.arm64_Release.depParamRefs["env.GIT_TAG_HASH"]}"
            }
        }
        dependency(Windows.Debug) {
            snapshot {
                onDependencyFailure = FailureAction.FAIL_TO_START
            }

            artifacts {
                artifactRules = "artifact.zip!**=>%perforce_path_to_publish_into%/${Windows.Debug.depParamRefs["env.GIT_TAG_HASH"]}"
            }
        }
        dependency(Windows.Internal) {
            snapshot {
                onDependencyFailure = FailureAction.FAIL_TO_START
            }

            artifacts {
                artifactRules = "artifact.zip!**=>%perforce_path_to_publish_into%/${Windows.Internal.depParamRefs["env.GIT_TAG_HASH"]}"
            }
        }
        dependency(Windows.Release) {
            snapshot {
                onDependencyFailure = FailureAction.FAIL_TO_START
            }

            artifacts {
                artifactRules = "artifact.zip!**=>%perforce_path_to_publish_into%/${Windows.Release.depParamRefs["env.GIT_TAG_HASH"]}"
            }
        }
        dependency(Windows.TrinityDev) {
            snapshot {
                onDependencyFailure = FailureAction.FAIL_TO_START
            }

            artifacts {
                artifactRules = "artifact.zip!**=>%perforce_path_to_publish_into%/${Windows.TrinityDev.depParamRefs["env.GIT_TAG_HASH"]}"
            }
        }
        artifacts(AbsoluteId("Infrastructure_MetaTeamCity_Tools_TeamcityChanges")) {
            buildRule = lastSuccessful()
            artifactRules = "binaries.zip!*=>"
        }
    }

    requirements {
        contains("teamcity.agent.jvm.os.name", "Windows")
        doesNotContain("teamcity.agent.name", "TEST-AUTOM")
    }
})

val PublishToPerforce = Publish("vendor/github.com/carbonengine/trinity")