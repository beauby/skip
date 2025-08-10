#!/bin/sh

SERVICE_SUBNET=subnet-0cf2b6b57f8e57324
CLIENTS_SUBNET=subnet-0cf2b6b57f8e57324
SERVICE_SECURITY_GROUP=sg-09eec6a7d5820f116
CLIENTS_SECURITY_GROUP=sg-05e1c021ec80580a4

COMMIT_HASH=$(git rev-parse --short HEAD)

export AWS_PAGER=""

# echo "Building skiplang base image..."
# docker build -t skiplabs/skip:${COMMIT_HASH} ../../ || exit 1
# echo -e "Done building skiplang base image.\n"

# echo "Building load test base image..."
# docker build \
#        --build-arg COMMIT_HASH=${COMMIT_HASH} \
#        -f ./Dockerfile \
#        -t skiplabs/skip-load-tests-base:${COMMIT_HASH} \
#        ../../ || exit 1
# echo -e "Done building load test base image.\n"

for t in $(find ./tests -mindepth 1 -maxdepth 1 -type d)
do
    test_name=$(basename "$t")
    echo "Building docker image for '${test_name}'..."
    image_name="skip-load-tests_${test_name}"
    # docker build \
    #        --build-arg COMMIT_HASH=${COMMIT_HASH} \
    #        -t "skiplabs/${image_name}:${COMMIT_HASH}" \
    #        "$t/server" || exit 1
    # docker push "skiplabs/${image_name}:${COMMIT_HASH}" || exit 1

    ecs_task_template="ecs-task-template.json"
    ecs_task="ecs-task.json"
    sed "s/{{IMAGE}}/$image_name/g" $ecs_task_template | sed "s/{{TAG}}/$COMMIT_HASH/g" > $ecs_task || exit 1
    task_def_revision=$(aws ecs register-task-definition \
                            --cli-input-json file://$ecs_task \
                            --query 'taskDefinition.revision' \
                            --output text) || exit 1
    (
        # ,securityGroups=[sg-0d869cd6a246b609e]
        task_arn=$(aws ecs run-task \
                       --cluster artilleryio-cluster \
                       --launch-type FARGATE \
                       --task-definition skip-load-testing-service:${task_def_revision} \
                       --network-configuration "awsvpcConfiguration={subnets=[$SERVICE_SUBNET],securityGroups=[$SERVICE_SECURITY_GROUP],assignPublicIp=DISABLED}" \
                       --query 'tasks[0].taskArn' \
                       --output text) || exit 1
        (
            echo "Waiting for service to be online..."
            aws ecs wait tasks-running \
                --cluster artilleryio-cluster \
                --tasks "$task_arn" || exit 1
            aws ecs describe-tasks \
                         --cluster artilleryio-cluster \
                         --tasks $task_arn
            eni_id=$(aws ecs describe-tasks \
                         --cluster artilleryio-cluster \
                         --tasks $task_arn \
                         --query "tasks[0].attachments[0].details[?name=='networkInterfaceId'].value" \
                         --output text) || exit 1
            echo "eni id: " $eni_id
            private_ip=$(aws ec2 describe-network-interfaces \
                             --network-interface-ids $eni_id \
                             --query "NetworkInterfaces[0].PrivateIpAddress" \
                             --output text) || exit 1
            echo "Done (${private_ip})"
            cd $t
            sed "s/{{CONTROL_URL}}/${private_ip}/g" test.yml | sed "s/{{STREAM_URL}}/${private_ip}/g" > test-patched.yml || exit 1
            npx artillery run-fargate test-patched.yml \
                -r eu-west-3 \
                --cluster artilleryio-cluster \
                --count 10 \
                --output=results.json \
                --security-group-ids $CLIENTS_SECURITY_GROUP \
                --subnet-ids $CLIENTS_SUBNET
        )
        aws ecs stop-task --cluster artilleryio-cluster --task "$task_arn"
    )
    aws ecs deregister-task-definition --task-definition  skip-load-testing-service:${task_def_revision}
done
