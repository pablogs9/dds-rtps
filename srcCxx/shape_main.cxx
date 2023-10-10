/****************************************************************
 * Use and redistribution is source and binary forms is permitted
 * subject to the OMG-DDS INTEROPERABILITY TESTING LICENSE found
 * at the following URL:
 *
 * https://github.com/omg-dds/dds-rtps/blob/master/LICENSE.md
 */
/****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>
#include <iostream>

#if defined(RTI_CONNEXT_DDS)
#include "shape_configurator_rti_connext_dds.h"
#elif defined(TWINOAKS_COREDX)
#include "shape_configurator_toc_coredx_dds.h"
#elif defined(OPENDDS)
#include "shape_configurator_opendds.h"
#elif defined(EPROSIMA_FAST_DDS)
#include "shape_configurator_eprosima_fast_dds.h"
#elif defined(EPROSIMA_SAFE_DDS)
#include "shape_configurator_eprosima_safe_dds.h"
#else
#error "Must define the DDS vendor"
#endif

#ifndef STRING_IN
#define STRING_IN
#endif
#ifndef STRING_INOUT
#define STRING_INOUT
#endif
#ifndef NAME_ACCESSOR
#define NAME_ACCESSOR
#endif
#ifndef LISTENER_STATUS_MASK_NONE
#define LISTENER_STATUS_MASK_NONE 0
#endif
#ifndef SECONDS_FIELD_NAME
#define SECONDS_FIELD_NAME seconds
#endif
#ifndef FIELD_ACCESSOR
#define FIELD_ACCESSOR
#endif
#ifndef GET_TOPIC_DESCRIPTION
#define GET_TOPIC_DESCRIPTION(dr) dr.get_topicdescription()
#endif
#ifndef ADD_PARTITION
#define ADD_PARTITION(field, value) StringSeq_push(field.name, value)
#endif

using namespace DDS;



/*************************************************************/
int  all_done  = 0;
/*************************************************************/
void
handle_sig(int sig)
{
    if (sig == SIGINT) {
        all_done = 1;
    }
}

/*************************************************************/
int
install_sig_handlers()
{
    struct sigaction int_action;
    int_action.sa_handler = handle_sig;
    sigemptyset(&int_action.sa_mask);
    sigaddset(&int_action.sa_mask, SIGINT);
    int_action.sa_flags     = 0;
    sigaction(SIGINT,  &int_action, NULL);
    return 0;
}

enum Verbosity
{
    ERROR=1,
    DEBUG=2,
};

class QosUtils {
public:
    static std::string to_string(ReliabilityQosPolicyKind reliability_value)
    {
        if (reliability_value == BEST_EFFORT_RELIABILITY_QOS){
            return "BEST_EFFORT";
        } else if (reliability_value == RELIABLE_RELIABILITY_QOS){
            return "RELIABLE";
        }
        return "Error stringifying Reliability kind.";
    }

    static std::string to_string(DurabilityQosPolicyKind durability_value)
    {
        if ( durability_value == VOLATILE_DURABILITY_QOS){
            return "VOLATILE";
        } else if (durability_value == TRANSIENT_LOCAL_DURABILITY_QOS){
            return "TRANSIENT_LOCAL";
        } else if (durability_value == TRANSIENT_DURABILITY_QOS){
            return "TRANSIENT";
        } else if (durability_value == PERSISTENT_DURABILITY_QOS){
            return "PERSISTENT";
        }
        return "Error stringifying Durability kind.";
    }

    static std::string to_string(DataRepresentationId_t data_representation_value)
    {
        if (data_representation_value == XCDR_DATA_REPRESENTATION){
            return "XCDR";
        } else if (data_representation_value == XCDR2_DATA_REPRESENTATION){
            return "XCDR2";
        }
        return "Error stringifying DataRepresentation.";
    }

    static std::string to_string(Verbosity verbosity_value)
    {
        switch (verbosity_value)
        {
        case ERROR:
            return "ERROR";
            break;

        case DEBUG:
            return "DEBUG";
            break;

        default:
            break;
        }
        return "Error stringifying verbosity.";
    }

    static std::string to_string(OwnershipQosPolicyKind ownership_kind_value)
    {
        if (ownership_kind_value == OwnershipQosPolicyKind::SHARED_OWNERSHIP_QOS){
            return "SHARED";
        } else if (ownership_kind_value == OwnershipQosPolicyKind::EXCLUSIVE_OWNERSHIP_QOS){
            return "EXCLUSIVE";
        }
        return "Error stringifying Ownership kind.";
    }

    static std::string to_string(HistoryQosPolicyKind history_kind_value)
    {
        if (history_kind_value == KEEP_ALL_HISTORY_QOS){
            return "KEEP_ALL";
        } else if (history_kind_value == KEEP_LAST_HISTORY_QOS){
            return "KEEP_LAST";
        }
        return "Error stringifying History kind.";
    }
};

class Logger{
public:
    Logger(enum Verbosity v)
    {
        verbosity_ = v;
    }

    void verbosity(enum Verbosity v)
    {
        verbosity_ = v;
    }

    enum Verbosity verbosity()
    {
        return verbosity_;
    }

    void log_message(std::string message, enum Verbosity level_verbosity)
    {
        if (level_verbosity <= verbosity_) {
            std::cout << message << std::endl;
        }
    }

private:
    enum Verbosity verbosity_;
};

/*************************************************************/
Logger logger(ERROR);
/*************************************************************/
class ShapeOptions {
public:
    DomainId_t                     domain_id;
    ReliabilityQosPolicyKind       reliability_kind;
    DurabilityQosPolicyKind        durability_kind;
    DataRepresentationId_t         data_representation;
    int                            history_depth;
    int                            ownership_strength;

    char               *topic_name;
    char               *color;
    char               *partition;

    bool                publish;
    bool                subscribe;

    int                 timebasedfilter_interval;
    int                 deadline_interval;

    int                 da_width;
    int                 da_height;

    int                 xvel;
    int                 yvel;
    int                 shapesize;

    bool                print_writer_samples;

public:
    //-------------------------------------------------------------
    ShapeOptions()
    {
        domain_id           = 0;
        reliability_kind    = RELIABLE_RELIABILITY_QOS;
        durability_kind     = VOLATILE_DURABILITY_QOS;
        data_representation = XCDR_DATA_REPRESENTATION;
        history_depth       = -1;      /* means default */
        ownership_strength  = -1;      /* means shared */

        topic_name         = NULL;
        color              = NULL;
        partition          = NULL;

        publish            = false;
        subscribe          = false;

        timebasedfilter_interval = 0; /* off */
        deadline_interval        = 0; /* off */

        da_width  = 240;
        da_height = 270;

        xvel = 3;
        yvel = 3;
        shapesize = 20;

        print_writer_samples = false;
    }

    //-------------------------------------------------------------
    ~ShapeOptions()
    {
        if (topic_name)  free(topic_name);
        if (color)       free(color);
        if (partition)   free(partition);
    }

    //-------------------------------------------------------------
    void print_usage( const char *prog )
    {
        printf("%s: \n", prog);
        printf("   -d <int>        : domain id (default: 0)\n");
        printf("   -b              : BEST_EFFORT reliability\n");
        printf("   -r              : RELIABLE reliability\n");
        printf("   -k <depth>      : keep history depth [0: KEEP_ALL]\n");
        printf("   -f <interval>   : set a 'deadline' with interval (seconds) [0: OFF]\n");
        printf("   -i <interval>   : apply 'time based filter' with interval (seconds) [0: OFF]\n");
        printf("   -s <int>        : set ownership strength [-1: SHARED]\n");
        printf("   -t <topic_name> : set the topic name\n");
        printf("   -c <color>      : set color to publish (filter if subscriber)\n");
        printf("   -p <partition>  : set a 'partition' string\n");
        printf("   -D [v|l|t|p]    : set durability [v: VOLATILE,  l: TRANSIENT_LOCAL]\n");
        printf("                                     t: TRANSIENT, p: PERSISTENT]\n");
        printf("   -P              : publish samples\n");
        printf("   -S              : subscribe samples\n");
        printf("   -x [1|2]        : set data representation [1: XCDR, 2: XCDR2]\n");
        printf("   -w              : print Publisher's samples\n");
        printf("   -z <int>        : set shapesize (between 10-99)\n");
        printf("   -v [e|d]        : set log message verbosity [e: ERROR, d: DEBUG]\n");
    }

    //-------------------------------------------------------------
    bool validate() {
        if (topic_name == NULL) {
            logger.log_message("please specify topic name [-t]", Verbosity::ERROR);
            return false;
        }
        if ( (!publish) && (!subscribe) ) {
            logger.log_message("please specify publish [-P] or subscribe [-S]", Verbosity::ERROR);
            return false;
        }
        if ( publish && subscribe ) {
            logger.log_message("please specify only one of: publish [-P] or subscribe [-S]", Verbosity::ERROR);
            return false;
        }
        if (publish && (color == NULL) ) {
            color = strdup("BLUE");
            logger.log_message("warning: color was not specified, defaulting to \"BLUE\"", Verbosity::ERROR);
        }
        return true;
    }

    //-------------------------------------------------------------
    bool parse(int argc, char *argv[])
    {
        int opt;
        bool parse_ok = true;
        // double d;
        while ((opt = getopt(argc, argv, "hbrc:d:D:f:i:k:p:s:x:t:v:z:wPS")) != -1)
        {
            switch (opt)
            {
            case 'v':
                {
                    if (optarg[0] != '\0')
                    {
                        switch (optarg[0])
                        {
                        case 'd':
                            {
                                logger.verbosity(DEBUG);
                                break;
                            }
                        case 'e':
                            {
                                logger.verbosity(ERROR);
                                break;
                            }
                        default:
                            {
                                logger.log_message("unrecognized value for verbosity "
                                                + std::string(1, optarg[0]),
                                        Verbosity::ERROR);
                                parse_ok = false;
                            }
                        }
                    }
                    break;
                }
            case 'w':
                {
                    print_writer_samples = true;
                    break;
                }
            case 'b':
                {
                    reliability_kind = BEST_EFFORT_RELIABILITY_QOS;
                    break;
                }
            case 'c':
                {
                    color = strdup(optarg);
                    break;
                }
            case 'd':
                {
                    int converted_param = sscanf(optarg, "%d", &domain_id);
                    if (converted_param == 0) {
                        logger.log_message("unrecognized value for domain_id "
                                        + std::string(1, optarg[0]),
                                Verbosity::ERROR);
                        parse_ok = false;
                    }
                    else if (domain_id < 0) {
                        logger.log_message("incorrect value for domain_id "
                                        + std::to_string(domain_id),
                                Verbosity::ERROR);
                        parse_ok = false;
                    }
                    break;
                }
            case 'D':
                {
                if (optarg[0] != '\0')
                {
                    switch (optarg[0])
                    {
                    case 'v':
                        {
                            durability_kind = VOLATILE_DURABILITY_QOS;
                            break;
                        }
                    case 'l':
                        {
                            durability_kind = TRANSIENT_LOCAL_DURABILITY_QOS;
                            break;
                        }
                    case 't':
                        {
                            durability_kind = TRANSIENT_DURABILITY_QOS;
                            break;
                        }
                    case 'p':
                        {
                            durability_kind = PERSISTENT_DURABILITY_QOS;
                            break;
                        }
                    default:
                        {
                            logger.log_message("unrecognized value for durability "
                                            + std::string(1, optarg[0]),
                                    Verbosity::ERROR);
                            parse_ok = false;
                        }
                    }
                }
                break;
                }
            case 'i':
                {
                    int converted_param = sscanf(optarg, "%d", &timebasedfilter_interval);
                    if (converted_param == 0) {
                        logger.log_message("unrecognized value for timebasedfilter_interval "
                                        + std::string(1, optarg[0]),
                                Verbosity::ERROR);
                        parse_ok = false;
                    }
                    else if (timebasedfilter_interval < 0) {
                        logger.log_message("incorrect value for timebasedfilter_interval "
                                        + std::to_string(timebasedfilter_interval),
                                Verbosity::ERROR);
                        parse_ok = false;
                    }
                    break;
                }
            case 'f':
                {
                    int converted_param = sscanf(optarg, "%d", &deadline_interval);
                    if (converted_param == 0) {
                        logger.log_message("unrecognized value for deadline_interval "
                                        + std::string(1, optarg[0]),
                                Verbosity::ERROR);
                        parse_ok = false;
                    } else if (deadline_interval < 0) {
                        logger.log_message("incorrect value for deadline_interval "
                                        + std::to_string(deadline_interval),
                                Verbosity::ERROR);
                        parse_ok = false;
                    }
                    break;
                }
            case 'k':
                {
                    int converted_param = sscanf(optarg, "%d", &history_depth);
                    if (converted_param == 0){
                        logger.log_message("unrecognized value for history_depth "
                                        + std::string(1, optarg[0]),
                                Verbosity::ERROR);
                        parse_ok = false;
                    } else if (history_depth < 0) {
                        logger.log_message("incorrect value for history_depth "
                                        + std::to_string(history_depth),
                                Verbosity::ERROR);
                        parse_ok = false;
                    }
                    break;
                }
            case 'p':
                {
                    partition = strdup(optarg);
                    break;
                }
            case 'r':
                {
                    reliability_kind = RELIABLE_RELIABILITY_QOS;
                    break;
                }
            case 's':
                {
                    int converted_param = sscanf(optarg, "%d", &ownership_strength);
                    if (converted_param == 0){
                        logger.log_message("unrecognized value for ownership_strength "
                                        + std::string(1, optarg[0]),
                                Verbosity::ERROR);
                        parse_ok = false;
                    } else if (ownership_strength < -1) {
                        logger.log_message("incorrect value for ownership_strength "
                                        + std::to_string(ownership_strength),
                                Verbosity::ERROR);
                        parse_ok = false;
                    }
                    break;
                }
            case 't':
                {
                    topic_name = strdup(optarg);
                    break;
                }
            case 'P':
                {
                    publish = true;
                    break;
                }
            case 'S':
                {
                    subscribe = true;
                    break;
                }
            case 'h':
                {
                    print_usage(argv[0]);
                    exit(0);
                    break;
                }
            case 'x':
                {
                    if (optarg[0] != '\0')
                    {
                        switch (optarg[0])
                        {
                        case '1':
                            {
                                data_representation = XCDR_DATA_REPRESENTATION;
                                break;
                            }
                        case '2':
                            {
                                data_representation = XCDR2_DATA_REPRESENTATION;
                                break;
                            }
                        default:
                            {
                            logger.log_message("unrecognized value for data representation "
                                            + std::string(1, optarg[0]),
                                    Verbosity::ERROR);
                            parse_ok = false;
                            }
                        }
                    }
                    break;
                }
            case 'z':
                {
                    int converted_param = sscanf(optarg, "%d", &shapesize);
                    if (converted_param == 0){
                        logger.log_message("unrecognized value for shapesize "
                                        + std::string(1, optarg[0]),
                                Verbosity::ERROR);
                        parse_ok = false;
                    }
                    else if (shapesize < 10 || shapesize > 99) {
                        logger.log_message("incorrect value for shapesize "
                                        + std::to_string(shapesize),
                                Verbosity::ERROR);
                        parse_ok = false;
                    }
                    break;
                }
            case '?':
                {
                    parse_ok = false;
                    break;
                }
            }

        }

        if ( parse_ok ) {
            parse_ok = validate();
        }
        if ( !parse_ok ) {
            print_usage(argv[0]);
        } else {
            std::string app_kind = publish ? "publisher" : "subscriber";
            logger.log_message("Shape Options: "
                    "\n    This application is a " + app_kind +
                    "\n    DomainId = " + std::to_string(domain_id) +
                    "\n    ReliabilityKind = " + QosUtils::to_string(reliability_kind) +
                    "\n    DurabilityKind = " + QosUtils::to_string(durability_kind) +
                    "\n    DataRepresentation = " + QosUtils::to_string(data_representation) +
                    "\n    HistoryDepth = " + std::to_string(history_depth) +
                    "\n    OwnershipStrength = " + std::to_string(ownership_strength) +
                    "\n    TimeBasedFilterInterval = " + std::to_string(timebasedfilter_interval) +
                    "\n    DeadlineInterval = " + std::to_string(deadline_interval) +
                    "\n    Shapesize = " + std::to_string(shapesize) +
                    "\n    Verbosity = " + QosUtils::to_string(logger.verbosity()),
                    Verbosity::DEBUG);
            if (topic_name != NULL){
                logger.log_message("    Topic = " + std::string(topic_name),
                        Verbosity::DEBUG);
            }
            if (color != NULL) {
                logger.log_message("    Color = " + std::string(color),
                        Verbosity::DEBUG);
            }
            if (partition != NULL) {
                logger.log_message("    Partition = " + std::string(partition), Verbosity::DEBUG);
            }
        }
        return parse_ok;
    }
};

/*************************************************************/
class DPListener : public DomainParticipantListener
{
public:
    void on_inconsistent_topic         (Topic &topic,  const InconsistentTopicStatus&) noexcept override {
        const char *topic_name = topic.get_name() NAME_ACCESSOR;
        const char *type_name  = topic.get_type_name() NAME_ACCESSOR;
        printf("%s() topic: '%s'  type: '%s'\n", __FUNCTION__, topic_name, type_name);
    }

    void on_offered_incompatible_qos(DataWriter &dw,  const OfferedIncompatibleQosStatus & status) noexcept override {
        Topic      &topic       = dw.get_topic( );
        const char *topic_name  = topic.get_name() NAME_ACCESSOR;
        const char *type_name   = topic.get_type_name() NAME_ACCESSOR;
        const char *policy_name = NULL;
        policy_name = get_qos_policy_name(status.last_policy_id);
        printf("%s() topic: '%s'  type: '%s' : %d (%s)\n", __FUNCTION__,
                topic_name, type_name,
                (uint32_t) status.last_policy_id,
                policy_name );
    }

    void on_publication_matched (DataWriter &dw, const PublicationMatchedStatus & status) noexcept override {
        Topic      &topic      = dw.get_topic( );
        const char *topic_name = topic.get_name() NAME_ACCESSOR;
        const char *type_name  = topic.get_type_name() NAME_ACCESSOR;
        printf("%s() topic: '%s'  type: '%s' : matched readers %d (change = %d)\n", __FUNCTION__,
                topic_name, type_name, status.current_count, status.current_count_change);
    }

    void on_offered_deadline_missed (DataWriter &dw, const OfferedDeadlineMissedStatus & status) noexcept override {
        Topic      &topic      = dw.get_topic( );
        const char *topic_name = topic.get_name() NAME_ACCESSOR;
        const char *type_name  = topic.get_type_name() NAME_ACCESSOR;
        printf("%s() topic: '%s'  type: '%s' : (total = %d, change = %d)\n", __FUNCTION__,
                topic_name, type_name, status.total_count, status.total_count_change);
    }

    void on_liveliness_lost (DataWriter &dw, const LivelinessLostStatus & status) noexcept override {
        Topic      &topic      = dw.get_topic( );
        const char *topic_name = topic.get_name() NAME_ACCESSOR;
        const char *type_name  = topic.get_type_name() NAME_ACCESSOR;
        printf("%s() topic: '%s'  type: '%s' : (total = %d, change = %d)\n", __FUNCTION__,
                topic_name, type_name, status.total_count, status.total_count_change);
    }

    void on_requested_incompatible_qos (DataReader &dr, const RequestedIncompatibleQosStatus & status) noexcept override {
        const TopicDescription &td         = GET_TOPIC_DESCRIPTION(dr);
        const char       *topic_name = td.get_name() NAME_ACCESSOR;
        const char       *type_name  = td.get_type_name() NAME_ACCESSOR;
        const char *policy_name = NULL;
        policy_name = get_qos_policy_name(status.last_policy_id);
        printf("%s() topic: '%s'  type: '%s' : %d (%s)\n", __FUNCTION__,
                topic_name, type_name, (uint32_t) status.last_policy_id,
                policy_name);
    }

    void on_subscription_matched (DataReader &dr, const SubscriptionMatchedStatus & status) noexcept override {
        const TopicDescription &td         = GET_TOPIC_DESCRIPTION(dr);
        const char       *topic_name = td.get_name() NAME_ACCESSOR;
        const char       *type_name  = td.get_type_name() NAME_ACCESSOR;
        printf("%s() topic: '%s'  type: '%s' : matched writers %d (change = %d)\n", __FUNCTION__,
                topic_name, type_name, status.current_count, status.current_count_change);
    }

    void on_requested_deadline_missed (DataReader &dr, const RequestedDeadlineMissedStatus & status) noexcept override {
        const TopicDescription &td         = GET_TOPIC_DESCRIPTION(dr);
        const char       *topic_name = td.get_name() NAME_ACCESSOR;
        const char       *type_name  = td.get_type_name() NAME_ACCESSOR;
        printf("%s() topic: '%s'  type: '%s' : (total = %d, change = %d)\n", __FUNCTION__,
                topic_name, type_name, status.total_count, status.total_count_change);
    }

    void on_liveliness_changed (DataReader &dr, const LivelinessChangedStatus & status) noexcept override {
        const TopicDescription &td         = GET_TOPIC_DESCRIPTION(dr);
        const char       *topic_name = td.get_name() NAME_ACCESSOR;
        const char       *type_name  = td.get_type_name() NAME_ACCESSOR;
        printf("%s() topic: '%s'  type: '%s' : (alive = %d, not_alive = %d)\n", __FUNCTION__,
                topic_name, type_name, status.alive_count, status.not_alive_count);
    }

  void on_sample_rejected (DataReader &, const SampleRejectedStatus &) noexcept override {}
  void on_data_available (DataReader &) noexcept override { printf("data available\n");}
  void on_sample_lost (DataReader &, const SampleLostStatus &) noexcept override {}
  void on_data_on_readers (Subscriber &) noexcept override {}
};


/*************************************************************/
class ShapeApplication {

private:
    DPListener               dp_listener;

    DomainParticipantFactory *dpf;
    DomainParticipant        *dp;
    Publisher                *pub;
    Subscriber               *sub;
    Topic                    *topic;
    ShapeTypeDataReader      *dr;
    ShapeTypeDataWriter      *dw;

    char                     *color;

    int                        xvel;
    int                        yvel;
    int                        da_width;
    int                        da_height;

    // Safe DDS specific
    execution::ISpinnable * executor;
    ShapeTypeTypeSupport typesupport;

public:
    //-------------------------------------------------------------
    ShapeApplication()
    {
        dpf = NULL;
        dp  = NULL;

        pub = NULL;
        sub = NULL;
        color = NULL;
    }

    //-------------------------------------------------------------
    ~ShapeApplication()
    {
        if (dp)  dp->delete_contained_entities( );
        if (dpf) dpf->delete_participant( dp );

        if (color) free(color);
    }

    //-------------------------------------------------------------
    bool initialize(ShapeOptions *options)
    {
        DomainParticipantFactory *dpf = OBTAIN_DOMAIN_PARTICIPANT_FACTORY;
        if (dpf == NULL) {
            logger.log_message("failed to create participant factory (missing license?).", Verbosity::ERROR);
            return false;
        }
        logger.log_message("Participant Factory created", Verbosity::DEBUG);

        DomainParticipantQos dp_qos = PARTICIPANT_QOS_DEFAULT;
        dp_qos.wire_protocol_qos().announced_locator = transport::Locator::from_ipv4({127, 0, 0, 1}, 8888);


        dp = dpf->create_participant( options->domain_id, dp_qos, &dp_listener, LISTENER_STATUS_MASK_ALL );
        if (dp == NULL) {
            logger.log_message("failed to create participant (missing license?).", Verbosity::ERROR);
            return false;
        }
        logger.log_message("Participant created", Verbosity::DEBUG);

        static memory::container::StaticString<100> type_name("ShapeType");
        static memory::container::StaticString<100> topic_name(options->topic_name);
        typesupport.register_type(*dp, type_name);

        printf("Create topic: %s\n", options->topic_name );
        topic = dp->create_topic( topic_name, type_name, TOPIC_QOS_DEFAULT, NULL, LISTENER_STATUS_MASK_NONE);
        if (topic == NULL) {
            logger.log_message("failed to create topic", Verbosity::ERROR);
            return false;
        }

        bool creation_ret = false;
        if ( options->publish ) {
            creation_ret = init_publisher(options);
        }
        else {
            creation_ret = init_subscriber(options);
        }

        dp->enable();

        executor = dpf->create_default_executor();

        return creation_ret;
    }

    //-------------------------------------------------------------
    bool run(ShapeOptions *options)
    {
        if ( pub != NULL ) {
            return run_publisher(options);
        }
        else if ( sub != NULL ) {
            return run_subscriber();
        }

        return false;
    }

    //-------------------------------------------------------------
    bool init_publisher(ShapeOptions *options)
    {
        logger.log_message("Initializing Publisher", Verbosity::DEBUG);
        PublisherQos  pub_qos;
        DataWriterQos dw_qos;
        ShapeType     shape;

        dp->get_default_publisher_qos( pub_qos );
        if ( options->partition != NULL ) {
            ADD_PARTITION(pub_qos.partition, options->partition);
        }

        pub = dp->create_publisher(pub_qos, NULL, LISTENER_STATUS_MASK_NONE);
        if (pub == NULL) {
            logger.log_message("failed to create publisher", Verbosity::ERROR);
            return false;
        }
        logger.log_message("Publisher created", Verbosity::DEBUG);
        logger.log_message("Data Writer QoS:", Verbosity::DEBUG);
        pub->get_default_datawriter_qos( dw_qos );
        dw_qos.reliability FIELD_ACCESSOR.kind = options->reliability_kind;
        logger.log_message("    Reliability = " + QosUtils::to_string(dw_qos.reliability FIELD_ACCESSOR.kind), Verbosity::DEBUG);
        dw_qos.durability FIELD_ACCESSOR.kind  = options->durability_kind;
        logger.log_message("    Durability = " + QosUtils::to_string(dw_qos.durability FIELD_ACCESSOR.kind), Verbosity::DEBUG);

        if ( options->deadline_interval > 0 ) {
            dw_qos.deadline FIELD_ACCESSOR.period.SECONDS_FIELD_NAME = options->deadline_interval;
            dw_qos.deadline FIELD_ACCESSOR.period.nanoseconds  = 0;
        }
        logger.log_message("    DeadlinePeriod = " + std::to_string(dw_qos.deadline FIELD_ACCESSOR.period.SECONDS_FIELD_NAME), Verbosity::DEBUG);

        // options->history_depth < 0 means leave default value
        if ( options->history_depth > 0 )  {
            dw_qos.history FIELD_ACCESSOR.kind  = KEEP_LAST_HISTORY_QOS;
            dw_qos.history FIELD_ACCESSOR.depth = options->history_depth;
        }
        else if ( options->history_depth <= 0 ) {
            dw_qos.history FIELD_ACCESSOR.kind  = KEEP_ALL_HISTORY_QOS;
        }
        logger.log_message("    History = " + QosUtils::to_string(dw_qos.history FIELD_ACCESSOR.kind), Verbosity::DEBUG);
        if (dw_qos.history FIELD_ACCESSOR.kind == KEEP_LAST_HISTORY_QOS){
            logger.log_message("    HistoryDepth = " + std::to_string(dw_qos.history FIELD_ACCESSOR.depth), Verbosity::DEBUG);
        }

        printf("Create writer for topic: %s color: %s\n", options->topic_name, options->color );
        dw = dynamic_cast<ShapeTypeDataWriter *>(pub->create_datawriter( *topic, dw_qos, NULL, LISTENER_STATUS_MASK_NONE));

        if (dw == NULL) {
            logger.log_message("failed to create datawriter", Verbosity::ERROR);
            return false;
        }

        color = strdup(options->color);
        xvel = options->xvel;
        yvel = options->yvel;
        da_width  = options->da_width;
        da_height = options->da_height;
        logger.log_message("Data Writer created", Verbosity::DEBUG);
        logger.log_message("Color " + std::string(color), Verbosity::DEBUG);
        logger.log_message("xvel " + std::to_string(xvel), Verbosity::DEBUG);
        logger.log_message("yvel " + std::to_string(yvel), Verbosity::DEBUG);
        logger.log_message("da_width " + std::to_string(da_width), Verbosity::DEBUG);
        logger.log_message("da_height " + std::to_string(da_height), Verbosity::DEBUG);

        return true;
    }

    //-------------------------------------------------------------
    bool init_subscriber(ShapeOptions *options)
    {
        if (options->color != NULL)
        {
            color = strdup(options->color);
        }

        SubscriberQos sub_qos;
        DataReaderQos dr_qos;

        dp->get_default_subscriber_qos( sub_qos );
        if ( options->partition != NULL ) {
            ADD_PARTITION(sub_qos.partition, options->partition);
        }

        sub = dp->create_subscriber( sub_qos, NULL, LISTENER_STATUS_MASK_NONE );
        if (sub == NULL) {
            logger.log_message("failed to create subscriber", Verbosity::ERROR);
            return false;
        }
        logger.log_message("Subscriber created", Verbosity::DEBUG);
        logger.log_message("Data Reader QoS:", Verbosity::DEBUG);
        sub->get_default_datareader_qos( dr_qos );
        dr_qos.reliability FIELD_ACCESSOR.kind = options->reliability_kind;
        logger.log_message("    Reliability = " + QosUtils::to_string(dr_qos.reliability FIELD_ACCESSOR.kind), Verbosity::DEBUG);
        dr_qos.durability FIELD_ACCESSOR.kind  = options->durability_kind;
        logger.log_message("    Durability = " + QosUtils::to_string(dr_qos.durability FIELD_ACCESSOR.kind), Verbosity::DEBUG);

        if ( options->deadline_interval > 0 ) {
            dr_qos.deadline FIELD_ACCESSOR.period.SECONDS_FIELD_NAME = options->deadline_interval;
            dr_qos.deadline FIELD_ACCESSOR.period.nanoseconds  = 0;
        }
        logger.log_message("    DeadlinePeriod = " + std::to_string(dr_qos.deadline FIELD_ACCESSOR.period.SECONDS_FIELD_NAME), Verbosity::DEBUG);

        // options->history_depth < 0 means leave default value
        if ( options->history_depth > 0 )  {
            dr_qos.history FIELD_ACCESSOR.kind  = KEEP_LAST_HISTORY_QOS;
            dr_qos.history FIELD_ACCESSOR.depth = options->history_depth;
        }
        else if ( options->history_depth == 0 ) {
            dr_qos.history FIELD_ACCESSOR.kind  = KEEP_ALL_HISTORY_QOS;
        }
        logger.log_message("    History = " + QosUtils::to_string(dr_qos.history FIELD_ACCESSOR.kind), Verbosity::DEBUG);
        if (dr_qos.history FIELD_ACCESSOR.kind == KEEP_LAST_HISTORY_QOS){
            logger.log_message("    HistoryDepth = " + std::to_string(dr_qos.history FIELD_ACCESSOR.depth), Verbosity::DEBUG);
        }


        printf("Create reader for topic: %s\n", options->topic_name );
        dr = dynamic_cast<ShapeTypeDataReader *>(sub->create_datareader(*topic, dr_qos, NULL, LISTENER_STATUS_MASK_NONE));


        if (dr == NULL) {
            logger.log_message("failed to create datareader", Verbosity::ERROR);
            return false;
        }
        logger.log_message("Data Reader created", Verbosity::DEBUG);
        return true;
    }

    //-------------------------------------------------------------
    bool run_subscriber()
    {
        while ( ! all_done )  {
            ReturnCode_t     retval;

            auto* typed_reader = TypedDataReader<ShapeTypeTypeSupport>::downcast(*dr);

            ShapeTypeTypeSupport::DataType shape;
            SampleInfo sample_info = {};

            do {

                retval = typed_reader->take_next_sample( shape, sample_info );
                if (retval == RETCODE_OK) {
                    bool color_match = true;
                    if (color != NULL)
                    {
                        color_match = strcmp(shape.color.c_str(), color) == 0;
                    }

                    if (sample_info.valid_data && color_match)  {
                        printf("%-10s %-10s %03d %03d [%d]\n", dr->get_topicdescription().get_name() NAME_ACCESSOR,
                                shape.color.c_str(),
                                shape.x,
                                shape.y,
                                shape.shapesize );
                    }
                }
            } while (retval == RETCODE_OK);

            while(executor->has_pending_work()){
                executor->spin(execution::TIME_ZERO);
            }

            get_transport().spin(get_platform().get_current_timepoint() + execution::TimePeriod::from_ms(100));
        }

        return true;
    }

    //-------------------------------------------------------------
    void
    moveShape( ShapeType *shape)
    {
        int w2;

        w2 = 1 + shape->shapesize / 2;
        shape->x = shape->x + xvel;
        shape->y = shape->y + yvel;
        if (shape->x < w2) {
            shape->x = w2;
            xvel = -xvel;
        }
        if (shape->x > da_width - w2) {
            shape->x = (da_width - w2);
            xvel = -xvel;
        }
        if (shape->y < w2) {
            shape->y = w2;
            yvel = -yvel;
        }
        if (shape->y > (da_height - w2) )  {
            shape->y = (da_height - w2);
            yvel = -yvel;
        }
    }

    //-------------------------------------------------------------
    bool run_publisher(ShapeOptions *options)
    {
        ShapeType shape;
#if defined(RTI_CONNEXT_DDS)
        ShapeType_initialize(&shape);
#endif

        srandom((uint32_t)time(NULL));

#ifndef STRING_ALLOC
#define STRING_ALLOC(A, B)
#endif
        STRING_ALLOC(shape.color, std::strlen(color));
#ifndef STRING_ASSIGN
        shape.color = color;
#else
        STRING_ASSIGN(shape.color, color);
#endif

        shape.shapesize = options->shapesize;
        shape.x =  random() % da_width;
        shape.y =  random() % da_height;
        xvel                   =  ((random() % 5) + 1) * ((random()%2)?-1:1);
        yvel                   =  ((random() % 5) + 1) * ((random()%2)?-1:1);;

        // bool matched_writer = false;

        while ( ! all_done )  {
            // if (!matched_writer)
            // {
            //     PublicationMatchedStatus status = {};
            //     dw->get_publication_matched_status(status);

            //     matched_writer = status.current_count >= 1;
            // }
            // else
            // {
                moveShape(&shape);

                // Typed Entities
                auto& writer = *(TypedDataWriter<ShapeTypeTypeSupport>::downcast(*dw));
                writer.write(shape, HANDLE_NIL);

                if (options->print_writer_samples)
                    printf("%-10s %-10s %03d %03d [%d]\n", dw->get_topic().get_name() NAME_ACCESSOR,
                                            shape.color.c_str(),
                                            shape.x,
                                            shape.y,
                                            shape.shapesize);
            // }

            while(executor->has_pending_work()){
                executor->spin(execution::TIME_ZERO);
            }
            get_transport().spin(get_platform().get_current_timepoint() + execution::TimePeriod::from_ms(33));

        }

        return true;
    }
};

/*************************************************************/
int main( int argc, char * argv[] )
{
    install_sig_handlers();

    ShapeOptions options;
    bool parseResult = options.parse(argc, argv);
    if ( !parseResult  ) {
        exit(1);
    }
    ShapeApplication shapeApp;
    if ( !shapeApp.initialize(&options) ) {
        exit(2);
    }
    if ( !shapeApp.run(&options) ) {
        exit(2);
    }

    printf("Done.\n");

    return 0;
}
