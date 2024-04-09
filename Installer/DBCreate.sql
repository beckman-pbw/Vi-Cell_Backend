--
-- PostgreSQL database cluster dump
--

-- Started on 2020-10-12 18:54:09

SET default_transaction_read_only = off;

SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;

--
-- Roles
--

CREATE ROLE "BCIDBAdmin";
ALTER ROLE "BCIDBAdmin" WITH SUPERUSER INHERIT CREATEROLE CREATEDB LOGIN REPLICATION NOBYPASSRLS PASSWORD 'md5c96dae1ec7c47ee007c6062dcb9f68de';
CREATE ROLE "BCIViCellAdmin";
ALTER ROLE "BCIViCellAdmin" WITH SUPERUSER INHERIT CREATEROLE CREATEDB LOGIN REPLICATION NOBYPASSRLS PASSWORD 'md54d89b4115b9bc6af7c00333751b98a9c';
CREATE ROLE "ViCellAdmin";
ALTER ROLE "ViCellAdmin" WITH SUPERUSER INHERIT CREATEROLE NOCREATEDB LOGIN REPLICATION NOBYPASSRLS PASSWORD 'md5a2bb58dd53bfc2264463571a146dbda7';
CREATE ROLE "ViCellDBAdmin";
ALTER ROLE "ViCellDBAdmin" WITH SUPERUSER INHERIT CREATEROLE NOCREATEDB LOGIN REPLICATION NOBYPASSRLS PASSWORD 'md5b6d313e926395f0d972047fde8a8369c';
CREATE ROLE "ViCellInstrumentUser";
ALTER ROLE "ViCellInstrumentUser" WITH NOSUPERUSER INHERIT NOCREATEROLE NOCREATEDB LOGIN NOREPLICATION NOBYPASSRLS PASSWORD 'md50868e831ec2366957977e9469e8f8d01';
CREATE ROLE "DbBackupUser";
ALTER ROLE "DbBackupUser" WITH NOSUPERUSER INHERIT NOCREATEROLE NOCREATEDB LOGIN REPLICATION NOBYPASSRLS PASSWORD 'md552faca163390d6518d8a96c2c73c8b7c';
CREATE ROLE postgres;


-- Completed on 2020-10-12 18:54:09

--
-- PostgreSQL database cluster dump complete
--

--
-- PostgreSQL database dump
--

-- Dumped from database version 10.12
-- Dumped by pg_dump version 10.12

-- Started on 2020-12-29 11:08:04

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET check_function_bodies = false;
SET xmloption = content;
SET client_min_messages = warning;
SET row_security = off;

DROP DATABASE "ViCellDB_template";
--
-- TOC entry 3305 (class 1262 OID 142311)
-- Name: ViCellDB_template; Type: DATABASE; Schema: -; Owner: BCIViCellAdmin
--

CREATE DATABASE "ViCellDB_template" WITH TEMPLATE = template0 ENCODING = 'UTF8' LC_COLLATE = 'English_United States.1252' LC_CTYPE = 'English_United States.1252';

ALTER DATABASE "ViCellDB_template" OWNER TO "BCIViCellAdmin";

\connect "ViCellDB_template"

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET check_function_bodies = false;
SET xmloption = content;
SET client_min_messages = warning;
SET row_security = off;

--
-- TOC entry 9 (class 2615 OID 142312)
-- Name: ViCellData; Type: SCHEMA; Schema: -; Owner: BCIViCellAdmin
--

CREATE SCHEMA "ViCellData";


ALTER SCHEMA "ViCellData" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 4 (class 2615 OID 142313)
-- Name: ViCellInstrument; Type: SCHEMA; Schema: -; Owner: BCIViCellAdmin
--

CREATE SCHEMA "ViCellInstrument";


ALTER SCHEMA "ViCellInstrument" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 1 (class 3079 OID 12924)
-- Name: plpgsql; Type: EXTENSION; Schema: -; Owner: 
--

CREATE EXTENSION IF NOT EXISTS plpgsql WITH SCHEMA pg_catalog;


--
-- TOC entry 3310 (class 0 OID 0)
-- Dependencies: 1
-- Name: EXTENSION plpgsql; Type: COMMENT; Schema: -; Owner: 
--

COMMENT ON EXTENSION plpgsql IS 'PL/pgSQL procedural language';


--
-- TOC entry 588 (class 1247 OID 142316)
-- Name: ad_settings; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.ad_settings AS (
	servername character varying,
	server_addr character varying,
	port_number integer,
	base_dn character varying,
	enabled boolean
);


ALTER TYPE public.ad_settings OWNER TO "BCIViCellAdmin";

--
-- TOC entry 591 (class 1247 OID 142319)
-- Name: af_settings; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.af_settings AS (
	save_image boolean,
	coarse_start integer,
	coarse_end integer,
	coarse_step smallint,
	fine_range integer,
	fine_step smallint,
	sharpness_low_threshold integer
);


ALTER TYPE public.af_settings OWNER TO "BCIViCellAdmin";

--
-- TOC entry 673 (class 1247 OID 142322)
-- Name: analysis_input_params; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.analysis_input_params AS (
	key smallint,
	skey smallint,
	sskey smallint,
	value real,
	polarity smallint
);


ALTER TYPE public.analysis_input_params OWNER TO "BCIViCellAdmin";

--
-- TOC entry 676 (class 1247 OID 142325)
-- Name: blob_characteristics; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.blob_characteristics AS (
	key smallint,
	skey smallint,
	sskey smallint,
	value real
);


ALTER TYPE public.blob_characteristics OWNER TO "BCIViCellAdmin";

--
-- TOC entry 679 (class 1247 OID 142328)
-- Name: blob_point; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.blob_point AS (
	startx smallint,
	starty smallint
);


ALTER TYPE public.blob_point OWNER TO "BCIViCellAdmin";

--
-- TOC entry 682 (class 1247 OID 142331)
-- Name: blob_data; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.blob_data AS (
	blob_info public.blob_characteristics[],
	blob_center public.blob_point,
	blob_outline public.blob_point[]
);


ALTER TYPE public.blob_data OWNER TO "BCIViCellAdmin";

--
-- TOC entry 685 (class 1247 OID 142334)
-- Name: blob_info_array; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.blob_info_array AS (
	blob_index integer,
	blob_info_list public.blob_characteristics[]
);


ALTER TYPE public.blob_info_array OWNER TO "BCIViCellAdmin";

--
-- TOC entry 688 (class 1247 OID 142337)
-- Name: blob_outline_array; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.blob_outline_array AS (
	blob_index integer,
	blob_outline public.blob_point[]
);


ALTER TYPE public.blob_outline_array OWNER TO "BCIViCellAdmin";

--
-- TOC entry 691 (class 1247 OID 142340)
-- Name: blob_rect; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.blob_rect AS (
	start_x smallint,
	start_y smallint,
	width smallint,
	height smallint
);


ALTER TYPE public.blob_rect OWNER TO "BCIViCellAdmin";

--
-- TOC entry 694 (class 1247 OID 142343)
-- Name: cal_consumable; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.cal_consumable AS (
	label character varying,
	lot_id character varying,
	cal_type smallint,
	expiration_date timestamp without time zone,
	assay_value real
);


ALTER TYPE public.cal_consumable OWNER TO "BCIViCellAdmin";

--
-- TOC entry 697 (class 1247 OID 142346)
-- Name: cluster_data; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.cluster_data AS (
	cell_count smallint,
	cluster_polygon public.blob_point[],
	cluster_box_startx smallint,
	cluster_box_starty smallint,
	cluster_box_width smallint,
	cluster_box_height smallint
);


ALTER TYPE public.cluster_data OWNER TO "BCIViCellAdmin";

--
-- TOC entry 700 (class 1247 OID 142349)
-- Name: column_display_info; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.column_display_info AS (
	col_type smallint,
	col_position smallint,
	col_width smallint,
	visible boolean
);


ALTER TYPE public.column_display_info OWNER TO "BCIViCellAdmin";

--
-- TOC entry 703 (class 1247 OID 142352)
-- Name: email_settings; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.email_settings AS (
	server_addr character varying,
	port_number integer,
	authenticate boolean,
	username character varying,
	password character varying
);


ALTER TYPE public.email_settings OWNER TO "BCIViCellAdmin";

--
-- TOC entry 706 (class 1247 OID 142355)
-- Name: illuminator_info; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.illuminator_info AS (
	type smallint,
	index smallint
);


ALTER TYPE public.illuminator_info OWNER TO "BCIViCellAdmin";

--
-- TOC entry 709 (class 1247 OID 142358)
-- Name: input_config_params; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.input_config_params AS (
	config_params_enum smallint,
	config_value double precision
);


ALTER TYPE public.input_config_params OWNER TO "BCIViCellAdmin";

--
-- TOC entry 712 (class 1247 OID 142361)
-- Name: int16_map_pair; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.int16_map_pair AS (
	channel smallint,
	max_num_peaks smallint
);


ALTER TYPE public.int16_map_pair OWNER TO "BCIViCellAdmin";

--
-- TOC entry 715 (class 1247 OID 142364)
-- Name: language_info; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.language_info AS (
	language_id integer,
	language_name character varying,
	locale_tag character varying,
	active boolean
);


ALTER TYPE public.language_info OWNER TO "BCIViCellAdmin";

--
-- TOC entry 718 (class 1247 OID 142367)
-- Name: rfid_sim_info; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.rfid_sim_info AS (
	set_valid_tag_data boolean,
	total_tags smallint,
	main_bay_file character varying,
	door_left_file character varying,
	door_right_file character varying
);


ALTER TYPE public.rfid_sim_info OWNER TO "BCIViCellAdmin";

--
-- TOC entry 721 (class 1247 OID 142370)
-- Name: run_options_info; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.run_options_info AS (
	sample_set_name character varying,
	sample_name character varying,
	save_image_count smallint,
	save_nth_image smallint,
	results_export boolean,
	results_export_folder character varying,
	append_results_export boolean,
	append_results_export_folder character varying,
	result_filename character varying,
	result_folder character varying,
	auto_export_pdf boolean,
	csv_folder character varying,
	wash_type smallint,
	dilution smallint,
	bpqc_cell_type_index smallint
);


ALTER TYPE public.run_options_info OWNER TO "BCIViCellAdmin";

--
-- TOC entry 724 (class 1247 OID 142373)
-- Name: signature_info; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.signature_info AS (
	username character varying,
	short_tag character varying,
	long_tag character varying,
	signature_time timestamp without time zone,
	signature_hash character varying
);


ALTER TYPE public.signature_info OWNER TO "BCIViCellAdmin";

SET default_tablespace = '';

SET default_with_oids = false;

--
-- TOC entry 218 (class 1259 OID 142374)
-- Name: Analyses; Type: TABLE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellData"."Analyses" (
    "AnalysisIdNum" bigint NOT NULL,
    "AnalysisID" uuid NOT NULL,
    "SampleID" uuid,
    "ImageSetID" uuid,
    "SummaryResultID" uuid,
    "SResultID" uuid,
    "RunUserID" uuid,
    "AnalysisDate" timestamp without time zone,
    "InstrumentSN" character varying,
    "BioProcessID" uuid,
    "QcProcessID" uuid,
    "WorkflowID" uuid,
    "ImageSequenceCount" smallint,
    "ImageSequenceIDList" uuid[],
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellData"."Analyses" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 219 (class 1259 OID 142381)
-- Name: Analyses_AnalysisIdNum_seq; Type: SEQUENCE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellData"."Analyses_AnalysisIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellData"."Analyses_AnalysisIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3312 (class 0 OID 0)
-- Dependencies: 219
-- Name: Analyses_AnalysisIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellData"."Analyses_AnalysisIdNum_seq" OWNED BY "ViCellData"."Analyses"."AnalysisIdNum";


--
-- TOC entry 220 (class 1259 OID 142383)
-- Name: DetailedResults; Type: TABLE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellData"."DetailedResults" (
    "DetailedResultIdNum" bigint NOT NULL,
    "DetailedResultID" uuid NOT NULL,
    "SampleID" uuid NOT NULL,
    "ImageID" uuid NOT NULL,
    "AnalysisID" uuid NOT NULL,
    "OwnerID" uuid NOT NULL,
    "ResultDate" timestamp without time zone NOT NULL,
    "ProcessingStatus" smallint NOT NULL,
    "TotCumulativeImages" smallint,
    "TotalCellsGP" integer,
    "TotalCellsPOI" integer,
    "POIPopulationPercent" double precision,
    "CellConcGP" double precision,
    "CellConcPOI" double precision,
    "AvgDiamGP" double precision,
    "AvgDiamPOI" double precision,
    "AvgCircularityGP" double precision,
    "AvgCircularityPOI" double precision,
    "AvgSharpnessGP" double precision,
    "AvgSharpnessPOI" double precision,
    "AvgEccentricityGP" double precision,
    "AvgEccentricityPOI" double precision,
    "AvgAspectRatioGP" double precision,
    "AvgAspectRatioPOI" double precision,
    "AvgRoundnessGP" double precision,
    "AvgRoundnessPOI" double precision,
    "AvgRawCellSpotBrightnessGP" double precision,
    "AvgRawCellSpotBrightnessPOI" double precision,
    "AvgCellSpotBrightnessGP" double precision,
    "AvgCellSpotBrightnessPOI" double precision,
    "AvgBkgndIntensity" double precision,
    "TotalBubbleCount" integer,
    "LargeClusterCount" integer,
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellData"."DetailedResults" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 221 (class 1259 OID 142387)
-- Name: DetailedResults_DetailedResultIdNum_seq; Type: SEQUENCE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellData"."DetailedResults_DetailedResultIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellData"."DetailedResults_DetailedResultIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3315 (class 0 OID 0)
-- Dependencies: 221
-- Name: DetailedResults_DetailedResultIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellData"."DetailedResults_DetailedResultIdNum_seq" OWNED BY "ViCellData"."DetailedResults"."DetailedResultIdNum";


--
-- TOC entry 222 (class 1259 OID 142389)
-- Name: ImageReferences; Type: TABLE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellData"."ImageReferences" (
    "ImageIdNum" bigint NOT NULL,
    "ImageID" uuid NOT NULL,
    "ImageSequenceID" uuid NOT NULL,
    "ImageChannel" smallint NOT NULL,
    "ImageFileName" character varying,
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellData"."ImageReferences" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3317 (class 0 OID 0)
-- Dependencies: 222
-- Name: TABLE "ImageReferences"; Type: COMMENT; Schema: ViCellData; Owner: BCIViCellAdmin
--

COMMENT ON TABLE "ViCellData"."ImageReferences" IS 'References to images and image storage locations; raw images are not stored in the database;';


--
-- TOC entry 223 (class 1259 OID 142396)
-- Name: ImageReferences_ImageIdNum_seq; Type: SEQUENCE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellData"."ImageReferences_ImageIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellData"."ImageReferences_ImageIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3319 (class 0 OID 0)
-- Dependencies: 223
-- Name: ImageReferences_ImageIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellData"."ImageReferences_ImageIdNum_seq" OWNED BY "ViCellData"."ImageReferences"."ImageIdNum";


--
-- TOC entry 224 (class 1259 OID 142398)
-- Name: ImageResults; Type: TABLE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellData"."ImageResults" (
    "ResultIdNum" bigint NOT NULL,
    "ResultID" uuid NOT NULL,
    "SampleID" uuid NOT NULL,
    "ImageID" uuid NOT NULL,
    "AnalysisID" uuid NOT NULL,
    "ImageSeqNum" smallint NOT NULL,
    "DetailedResultID" uuid NOT NULL,
    "MaxNumOfPeaksFlChanMap" public.int16_map_pair[],
    "NumBlobs" smallint,
    "BlobInfoListStr" character varying,
    "BlobCenterListStr" character varying,
    "BlobOutlineListStr" character varying,
    "NumClusters" smallint,
    "ClusterCellCountList" smallint[],
    "ClusterPolygonListStr" character varying,
    "ClusterRectListStr" character varying,
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellData"."ImageResults" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 225 (class 1259 OID 142405)
-- Name: ImageResults_ResultIdNum_seq; Type: SEQUENCE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellData"."ImageResults_ResultIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellData"."ImageResults_ResultIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3322 (class 0 OID 0)
-- Dependencies: 225
-- Name: ImageResults_ResultIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellData"."ImageResults_ResultIdNum_seq" OWNED BY "ViCellData"."ImageResults"."ResultIdNum";


--
-- TOC entry 226 (class 1259 OID 142407)
-- Name: ImageSequences; Type: TABLE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellData"."ImageSequences" (
    "ImageSequenceIdNum" bigint NOT NULL,
    "ImageSequenceID" uuid NOT NULL,
    "ImageSetID" uuid NOT NULL,
    "SequenceNum" smallint NOT NULL,
    "ImageCount" smallint NOT NULL,
    "FlChannels" smallint NOT NULL,
    "ImageIDList" uuid[],
    "ImageSequenceFolder" character varying NOT NULL,
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellData"."ImageSequences" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3324 (class 0 OID 0)
-- Dependencies: 226
-- Name: TABLE "ImageSequences"; Type: COMMENT; Schema: ViCellData; Owner: BCIViCellAdmin
--

COMMENT ON TABLE "ViCellData"."ImageSequences" IS 'records of each image sequence (image composite) taken for a particular sequence number (typically, 1 - 100) for a sample; the record details if multiple images are taken at a given point in the image sequence (as would occur for fluorescence);';


--
-- TOC entry 227 (class 1259 OID 142414)
-- Name: ImageSequences_ImageSequenceIdNum_seq; Type: SEQUENCE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellData"."ImageSequences_ImageSequenceIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellData"."ImageSequences_ImageSequenceIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3326 (class 0 OID 0)
-- Dependencies: 227
-- Name: ImageSequences_ImageSequenceIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellData"."ImageSequences_ImageSequenceIdNum_seq" OWNED BY "ViCellData"."ImageSequences"."ImageSequenceIdNum";


--
-- TOC entry 228 (class 1259 OID 142416)
-- Name: ImageSets; Type: TABLE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellData"."ImageSets" (
    "ImageSetIdNum" bigint NOT NULL,
    "ImageSetID" uuid NOT NULL,
    "SampleID" uuid NOT NULL,
    "CreationDate" timestamp without time zone NOT NULL,
    "ImageSetFolder" character varying NOT NULL,
    "ImageSequenceCount" smallint NOT NULL,
    "ImageSequenceIDList" uuid[],
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellData"."ImageSets" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3328 (class 0 OID 0)
-- Dependencies: 228
-- Name: TABLE "ImageSets"; Type: COMMENT; Schema: ViCellData; Owner: BCIViCellAdmin
--

COMMENT ON TABLE "ViCellData"."ImageSets" IS 'Record documenting all images belonging to a sample';


--
-- TOC entry 229 (class 1259 OID 142423)
-- Name: ImageSets_ImageSetIdNum_seq; Type: SEQUENCE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellData"."ImageSets_ImageSetIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellData"."ImageSets_ImageSetIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3330 (class 0 OID 0)
-- Dependencies: 229
-- Name: ImageSets_ImageSetIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellData"."ImageSets_ImageSetIdNum_seq" OWNED BY "ViCellData"."ImageSets"."ImageSetIdNum";


--
-- TOC entry 230 (class 1259 OID 142425)
-- Name: SResults; Type: TABLE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellData"."SResults" (
    "SResultIdNum" bigint NOT NULL,
    "SResultID" uuid NOT NULL,
    "CumulativeDetailedResultID" uuid NOT NULL,
    "ImageResultIDList" uuid[] NOT NULL,
    "AnalysisID" uuid NOT NULL,
    "SampleID" uuid NOT NULL,
    "ProcessingSettingsID" uuid NOT NULL,
    "CumMaxNumOfPeaksFlChan" public.int16_map_pair[],
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellData"."SResults" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 231 (class 1259 OID 142432)
-- Name: SResults_SResultIdNum_seq; Type: SEQUENCE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellData"."SResults_SResultIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellData"."SResults_SResultIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3333 (class 0 OID 0)
-- Dependencies: 231
-- Name: SResults_SResultIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellData"."SResults_SResultIdNum_seq" OWNED BY "ViCellData"."SResults"."SResultIdNum";


--
-- TOC entry 232 (class 1259 OID 142434)
-- Name: SampleProperties; Type: TABLE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellData"."SampleProperties" (
    "SampleIdNum" bigint NOT NULL,
    "SampleID" uuid NOT NULL,
    "SampleStatus" smallint,
    "SampleName" character varying,
    "CellTypeID" uuid NOT NULL,
    "CellTypeIndex" integer,
    "AnalysisDefinitionID" uuid,
    "AnalysisDefinitionIndex" integer,
    "Label" character varying,
    "BioProcessID" uuid,
    "QcProcessID" uuid,
    "WorkflowID" uuid,
    "Comments" character varying,
    "WashType" smallint,
    "Dilution" smallint,
    "OwnerUserID" uuid,
    "RunUserID" uuid,
    "AcquisitionDate" timestamp without time zone,
    "ImageSetID" uuid,
    "DustRefImageSetID" uuid,
    "InstrumentSN" character varying,
    "ImageAnalysisParamID" uuid NOT NULL,
    "NumReagents" smallint,
    "ReagentTypeNameList" character varying[],
    "ReagentPackNumList" character varying[],
    "PackLotNumList" character varying[],
    "PackLotExpirationList" bigint[],
    "PackInServiceList" bigint[],
    "PackServiceExpirationList" bigint[],
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellData"."SampleProperties" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 233 (class 1259 OID 142441)
-- Name: SampleProperties_SampleIdNum_seq; Type: SEQUENCE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellData"."SampleProperties_SampleIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellData"."SampleProperties_SampleIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3336 (class 0 OID 0)
-- Dependencies: 233
-- Name: SampleProperties_SampleIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellData"."SampleProperties_SampleIdNum_seq" OWNED BY "ViCellData"."SampleProperties"."SampleIdNum";


--
-- TOC entry 234 (class 1259 OID 142443)
-- Name: SummaryResults; Type: TABLE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellData"."SummaryResults" (
    "SummaryResultIdNum" bigint NOT NULL,
    "SummaryResultID" uuid NOT NULL,
    "SampleID" uuid NOT NULL,
    "ImageSetID" uuid NOT NULL,
    "AnalysisID" uuid NOT NULL,
    "ResultDate" timestamp without time zone NOT NULL,
    "SignatureList" public.signature_info[],
    "ImageAnalysisParamID" uuid NOT NULL,
    "AnalysisDefID" uuid,
    "AnalysisParamID" uuid NOT NULL,
    "CellTypeID" uuid NOT NULL,
    "CellTypeIndex" integer,
    "ProcessingStatus" smallint NOT NULL,
    "TotCumulativeImages" smallint,
    "TotalCellsGP" integer,
    "TotalCellsPOI" integer,
    "POIPopulationPercent" real,
    "CellConcGP" real,
    "CellConcPOI" real,
    "AvgDiamGP" real,
    "AvgDiamPOI" real,
    "AvgCircularityGP" real,
    "AvgCircularityPOI" real,
    "CoefficientOfVariance" real,
    "AvgCellsPerImage" smallint,
    "AvgBkgndIntensity" smallint,
    "TotalBubbleCount" smallint,
    "LargeClusterCount" smallint,
    "QcStatus" smallint,	
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellData"."SummaryResults" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 235 (class 1259 OID 142450)
-- Name: SummaryResults_SummaryResultIdNum_seq; Type: SEQUENCE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellData"."SummaryResults_SummaryResultIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellData"."SummaryResults_SummaryResultIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3339 (class 0 OID 0)
-- Dependencies: 235
-- Name: SummaryResults_SummaryResultIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellData"."SummaryResults_SummaryResultIdNum_seq" OWNED BY "ViCellData"."SummaryResults"."SummaryResultIdNum";


--
-- TOC entry 236 (class 1259 OID 142452)
-- Name: AnalysisDefinitions; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."AnalysisDefinitions" (
    "AnalysisDefinitionIdNum" bigint NOT NULL,
    "AnalysisDefinitionID" uuid NOT NULL,
    "AnalysisDefinitionIndex" integer,
    "AnalysisDefinitionName" character varying,
    "NumReagents" smallint,
    "ReagentTypeIndexList" integer[],
    "MixingCycles" smallint,
    "NumIlluminators" smallint,
    "IlluminatorsIndexList" smallint[],
    "NumAnalysisParams" smallint,
    "AnalysisParamIDList" uuid[],
    "PopulationParamExists" boolean DEFAULT false NOT NULL,
    "PopulationParamID" uuid,
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellInstrument"."AnalysisDefinitions" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 237 (class 1259 OID 142460)
-- Name: AnalysisDefinitions_AnalysisDefinitionIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."AnalysisDefinitions_AnalysisDefinitionIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."AnalysisDefinitions_AnalysisDefinitionIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3342 (class 0 OID 0)
-- Dependencies: 237
-- Name: AnalysisDefinitions_AnalysisDefinitionIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."AnalysisDefinitions_AnalysisDefinitionIdNum_seq" OWNED BY "ViCellInstrument"."AnalysisDefinitions"."AnalysisDefinitionIdNum";


--
-- TOC entry 238 (class 1259 OID 142462)
-- Name: AnalysisInputSettings; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."AnalysisInputSettings" (
    "SettingsIdNum" bigint NOT NULL,
    "SettingsID" uuid NOT NULL,
    "InputConfigParamMap" public.input_config_params[],
    "CellIdentParamList" public.analysis_input_params[],
    "POIIdentParamList" public.analysis_input_params[]
);


ALTER TABLE "ViCellInstrument"."AnalysisInputSettings" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 239 (class 1259 OID 142468)
-- Name: AnalysisInputSettings_SettingsIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."AnalysisInputSettings_SettingsIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."AnalysisInputSettings_SettingsIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3345 (class 0 OID 0)
-- Dependencies: 239
-- Name: AnalysisInputSettings_SettingsIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."AnalysisInputSettings_SettingsIdNum_seq" OWNED BY "ViCellInstrument"."AnalysisInputSettings"."SettingsIdNum";


--
-- TOC entry 240 (class 1259 OID 142470)
-- Name: AnalysisParams; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."AnalysisParams" (
    "AnalysisParamIdNum" bigint NOT NULL,
    "AnalysisParamID" uuid NOT NULL,
    "IsInitialized" boolean,
    "AnalysisParamLabel" character varying,
    "CharacteristicKey" smallint,
    "CharacteristicSKey" smallint,
    "CharacteristicSSKey" smallint,
    "ThresholdValue" real,
    "AboveThreshold" boolean,
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellInstrument"."AnalysisParams" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 241 (class 1259 OID 142477)
-- Name: AnalysisParams_AnalysisParamIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."AnalysisParams_AnalysisParamIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."AnalysisParams_AnalysisParamIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3348 (class 0 OID 0)
-- Dependencies: 241
-- Name: AnalysisParams_AnalysisParamIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."AnalysisParams_AnalysisParamIdNum_seq" OWNED BY "ViCellInstrument"."AnalysisParams"."AnalysisParamIdNum";


--
-- TOC entry 242 (class 1259 OID 142479)
-- Name: BioProcesses; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."BioProcesses" (
    "BioProcessIdNum" bigint NOT NULL,
    "BioProcessID" uuid NOT NULL,
    "BioProcessName" character varying NOT NULL,
    "BioProcessSequence" character varying,
    "ReactorName" character varying,
    "CelltypeID" uuid,
    "CellTypeIndex" integer,
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellInstrument"."BioProcesses" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 243 (class 1259 OID 142486)
-- Name: BioProcesses_BioProcessIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."BioProcesses_BioProcessIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."BioProcesses_BioProcessIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3351 (class 0 OID 0)
-- Dependencies: 243
-- Name: BioProcesses_BioProcessIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."BioProcesses_BioProcessIdNum_seq" OWNED BY "ViCellInstrument"."BioProcesses"."BioProcessIdNum";


--
-- TOC entry 244 (class 1259 OID 142488)
-- Name: Calibrations; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."Calibrations" (
    "CalibrationIdNum" bigint NOT NULL,
    "CalibrationID" uuid NOT NULL,
    "InstrumentSN" character varying,
    "CalibrationDate" timestamp without time zone,
    "CalibrationUserID" uuid,
    "CalibrationType" smallint,
    "Slope" double precision,
    "Intercept" double precision,
    "ImageCount" smallint,
    "CalQueueID" uuid,
    "ConsumablesList" public.cal_consumable[],
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellInstrument"."Calibrations" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 245 (class 1259 OID 142495)
-- Name: Calibrations_CalibrationIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."Calibrations_CalibrationIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."Calibrations_CalibrationIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3354 (class 0 OID 0)
-- Dependencies: 245
-- Name: Calibrations_CalibrationIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."Calibrations_CalibrationIdNum_seq" OWNED BY "ViCellInstrument"."Calibrations"."CalibrationIdNum";


--
-- TOC entry 246 (class 1259 OID 142497)
-- Name: CellTypes; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."CellTypes" (
    "CellTypeIdNum" bigint NOT NULL,
    "CellTypeID" uuid NOT NULL,
    "CellTypeIndex" integer,
    "CellTypeName" character varying,
    "Retired" boolean DEFAULT false,
    "MaxImages" smallint,
    "AspirationCycles" smallint,
    "MinDiamMicrons" real,
    "MaxDiamMicrons" real,
    "MinCircularity" real,
    "SharpnessLimit" real,
    "NumCellIdentParams" smallint,
    "CellIdentParamIDList" uuid[],
    "DeclusterSetting" smallint,
    "RoiExtent" real,
    "RoiXPixels" integer,
    "RoiYPixels" integer,
    "NumAnalysisSpecializations" smallint,
    "AnalysisSpecializationIDList" uuid[],
    "CalculationAdjustmentFactor" real DEFAULT 0.0 NOT NULL,
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellInstrument"."CellTypes" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 247 (class 1259 OID 142506)
-- Name: CellTypes_CellTypeIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."CellTypes_CellTypeIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."CellTypes_CellTypeIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3357 (class 0 OID 0)
-- Dependencies: 247
-- Name: CellTypes_CellTypeIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."CellTypes_CellTypeIdNum_seq" OWNED BY "ViCellInstrument"."CellTypes"."CellTypeIdNum";


--
-- TOC entry 248 (class 1259 OID 142508)
-- Name: IlluminatorTypes; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."IlluminatorTypes" (
    "IlluminatorIdNum" bigint NOT NULL,
    "IlluminatorIndex" smallint NOT NULL,
    "IlluminatorType" smallint NOT NULL,
    "IlluminatorName" character varying NOT NULL,
    "PositionNum" smallint,
    "Tolerance" real DEFAULT 0.1,
    "MaxVoltage" integer DEFAULT 0,
    "IlluminatorWavelength" smallint,
    "EmissionWavelength" smallint,
    "ExposureTimeMs" smallint,
    "PercentPower" smallint,
    "SimmerVoltage" integer,
    "Ltcd" smallint DEFAULT 100,
    "Ctld" smallint DEFAULT 100,
    "FeedbackPhotoDiode" smallint DEFAULT 1,
    "Protected" boolean DEFAULT true NOT NULL
);


ALTER TABLE "ViCellInstrument"."IlluminatorTypes" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 249 (class 1259 OID 142520)
-- Name: IlluminatorTypes_IlluminatorIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."IlluminatorTypes_IlluminatorIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."IlluminatorTypes_IlluminatorIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3360 (class 0 OID 0)
-- Dependencies: 249
-- Name: IlluminatorTypes_IlluminatorIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."IlluminatorTypes_IlluminatorIdNum_seq" OWNED BY "ViCellInstrument"."IlluminatorTypes"."IlluminatorIdNum";


--
-- TOC entry 250 (class 1259 OID 142522)
-- Name: ImageAnalysisCellIdentParams; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."ImageAnalysisCellIdentParams" (
    "IdentParamIdNum" bigint NOT NULL,
    "IdentParamID" uuid NOT NULL,
    "CharacteristicKey" smallint,
    "CharacteristicSKey" smallint,
    "CharacteristicSSKey" smallint,
    "ParamValue" real,
    "ValueTest" smallint,
    "Protected" boolean DEFAULT true NOT NULL
);


ALTER TABLE "ViCellInstrument"."ImageAnalysisCellIdentParams" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 251 (class 1259 OID 142526)
-- Name: ImageAnalysisCellIdentParams_IdentParamIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."ImageAnalysisCellIdentParams_IdentParamIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."ImageAnalysisCellIdentParams_IdentParamIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3363 (class 0 OID 0)
-- Dependencies: 251
-- Name: ImageAnalysisCellIdentParams_IdentParamIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."ImageAnalysisCellIdentParams_IdentParamIdNum_seq" OWNED BY "ViCellInstrument"."ImageAnalysisCellIdentParams"."IdentParamIdNum";


--
-- TOC entry 252 (class 1259 OID 142528)
-- Name: ImageAnalysisParams; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."ImageAnalysisParams" (
    "ImageAnalysisParamIdNum" bigint NOT NULL,
    "ImageAnalysisParamID" uuid NOT NULL,
    "AlgorithmMode" integer,
    "BubbleMode" boolean,
    "DeclusterMode" boolean,
    "SubPeakAnalysisMode" boolean,
    "DilutionFactor" integer,
    "ROIXcoords" integer,
    "ROIYcoords" integer,
    "DeclusterAccumulatorThreshLow" integer,
    "DeclusterMinDistanceThreshLow" integer,
    "DeclusterAccumulatorThreshMed" integer,
    "DeclusterMinDistanceThreshMed" integer,
    "DeclusterAccumulatorThreshHigh" integer,
    "DeclusterMinDistanceThreshHigh" integer,
    "FovDepthMM" double precision,
    "PixelFovMM" double precision,
    "SizingSlope" double precision,
    "SizingIntercept" double precision,
    "ConcSlope" double precision,
    "ConcIntercept" double precision,
    "ConcImageControlCnt" integer,
    "BubbleMinSpotAreaPrcnt" real,
    "BubbleMinSpotAreaBrightness" real,
    "BubbleRejectImgAreaPrcnt" real,
    "VisibleCellSpotArea" double precision,
    "FlScalableROI" double precision,
    "FLPeakPercent" double precision,
    "NominalBkgdLevel" double precision,
    "BkgdIntensityTolerance" double precision,
    "CenterSpotMinIntensityLimit" double precision,
    "PeakIntensitySelectionAreaLimit" double precision,
    "CellSpotBrightnessExclusionThreshold" double precision,
    "HotPixelEliminationMode" double precision,
    "ImgBotAndRightBoundaryAnnotationMode" double precision,
    "SmallParticleSizingCorrection" double precision,
    "Protected" boolean DEFAULT true NOT NULL
);


ALTER TABLE "ViCellInstrument"."ImageAnalysisParams" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 253 (class 1259 OID 142532)
-- Name: ImageAnalysisParams_ImageAnalysisParamIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."ImageAnalysisParams_ImageAnalysisParamIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."ImageAnalysisParams_ImageAnalysisParamIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3366 (class 0 OID 0)
-- Dependencies: 253
-- Name: ImageAnalysisParams_ImageAnalysisParamIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."ImageAnalysisParams_ImageAnalysisParamIdNum_seq" OWNED BY "ViCellInstrument"."ImageAnalysisParams"."ImageAnalysisParamIdNum";


--
-- TOC entry 254 (class 1259 OID 142534)
-- Name: InstrumentConfig; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."InstrumentConfig" (
    "InstrumentIdNum" bigint NOT NULL,
    "InstrumentSN" character varying DEFAULT 'ViCellInstrumentSN'::character varying NOT NULL,
    "InstrumentType" smallint DEFAULT 1 NOT NULL,
    "DeviceName" character varying,
    "UIVersion" character varying,
    "SoftwareVersion" character varying,
    "AnalysisSWVersion" character varying,
    "FirmwareVersion" character varying,
    "BrightFieldLedType" smallint,
    "CameraType" smallint,
    "CameraFWVersion" character varying,
    "CameraConfig" character varying,
    "PumpType" smallint,
    "PumpFWVersion" character varying,
    "PumpConfig" character varying,
    "IlluminatorsInfoList" public.illuminator_info[],
    "IlluminatorConfig" character varying,
    "ConfigType" smallint,
    "LogName" character varying,
    "LogMaxSize" integer,
    "LogSensitivity" character varying,
    "MaxLogs" smallint,
    "AlwaysFlush" boolean,
    "CameraErrorLogName" character varying,
    "CameraErrorLogMaxSize" integer,
    "StorageErrorLogName" character varying,
    "StorageErrorLogMaxSize" integer,
    "CarouselThetaHomeOffset" integer DEFAULT 0,
    "CarouselRadiusOffset" integer DEFAULT 0,
    "PlateThetaHomePosOffset" integer DEFAULT 0,
    "PlateThetaCalPos" integer DEFAULT 0,
    "PlateRadiusCenterPos" integer DEFAULT 0,
    "SaveImage" smallint,
    "FocusPosition" integer,
    "AutoFocus" public.af_settings,
    "AbiMaxImageCount" smallint,
    "SampleNudgeVolume" smallint,
    "SampleNudgeSpeed" smallint,
    "FlowCellDepth" real,
    "FlowCellDepthConstant" real,
    "RfidSim" public.rfid_sim_info,
    "LegacyData" boolean,
    "CarouselSimulator" boolean,
    "NightlyCleanOffset" smallint,
    "LastNightlyClean" timestamp without time zone,
    "SecurityMode" smallint,
    "InactivityTimeout" smallint,
    "PasswordExpiration" smallint,
    "NormalShutdown" boolean,
    "NextAnalysisDefIndex" integer DEFAULT 0,
    "NextFactoryCellTypeIndex" integer DEFAULT 0,
    "NextUserCellTypeIndex" bigint DEFAULT '2147483648'::bigint,
    "SamplesProcessed" integer DEFAULT 0,
    "DiscardCapacity" smallint DEFAULT 120,
    "EmailServer" public.email_settings,
    "ADSettings" public.ad_settings,
    "LanguageList" public.language_info[],
    "DefaultDisplayColumns" public.column_display_info[],
    "RunOptionDefaults" public.run_options_info,
    "AutomationInstalled" boolean DEFAULT false,
    "AutomationEnabled" boolean DEFAULT false,
    "ACupEnabled" boolean DEFAULT false,
    "AutomationPort" integer DEFAULT 0,
    "Protected" boolean DEFAULT true NOT NULL
);


ALTER TABLE "ViCellInstrument"."InstrumentConfig" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 255 (class 1259 OID 142556)
-- Name: InstrumentConfig_InstrumentIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."InstrumentConfig_InstrumentIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."InstrumentConfig_InstrumentIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3369 (class 0 OID 0)
-- Dependencies: 255
-- Name: InstrumentConfig_InstrumentIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."InstrumentConfig_InstrumentIdNum_seq" OWNED BY "ViCellInstrument"."InstrumentConfig"."InstrumentIdNum";


--
-- TOC entry 256 (class 1259 OID 142558)
-- Name: QcProcesses; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."QcProcesses" (
    "QcIdNum" bigint NOT NULL,
    "QcID" uuid NOT NULL,
    "QcName" character varying NOT NULL,
    "QcType" smallint,
    "CellTypeID" uuid,
    "CellTypeIndex" integer,
    "LotInfo" character varying,
    "LotExpiration" timestamp without time zone,
    "AssayValue" double precision,
    "AllowablePercentage" double precision,
    "QcSequence" character varying,
    "Comments" character varying,
    "Retired" boolean DEFAULT false,
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellInstrument"."QcProcesses" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 257 (class 1259 OID 142565)
-- Name: QcProcesses_QcIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."QcProcesses_QcIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."QcProcesses_QcIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3372 (class 0 OID 0)
-- Dependencies: 257
-- Name: QcProcesses_QcIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."QcProcesses_QcIdNum_seq" OWNED BY "ViCellInstrument"."QcProcesses"."QcIdNum";


--
-- TOC entry 258 (class 1259 OID 142567)
-- Name: ReagentInfo; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."ReagentInfo" (
    "ReagentIdNum" bigint NOT NULL,
    "ReagentTypeNum" integer,
    "Current" boolean,
    "ContainerTagSN" character varying,
    "ReagentIndexList" smallint[],
    "ReagentNamesList" character varying[],
    "MixingCycles" smallint[],
    "PackPartNum" character varying,
    "LotNum" character varying,
    "LotExpiration" bigint,
    "InService" bigint,
    "ServiceLife" smallint,
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellInstrument"."ReagentInfo" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 259 (class 1259 OID 142574)
-- Name: ReagentInfo_ReagentIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."ReagentInfo_ReagentIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."ReagentInfo_ReagentIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3375 (class 0 OID 0)
-- Dependencies: 259
-- Name: ReagentInfo_ReagentIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."ReagentInfo_ReagentIdNum_seq" OWNED BY "ViCellInstrument"."ReagentInfo"."ReagentIdNum";


CREATE TABLE "ViCellInstrument"."CellHealthReagents" (
	"IdNum" bigint NOT NULL,
	"ID" uuid NOT NULL,
	"Type" smallint,
	"Name" character varying,
	"Volume" integer,
	"Protected" boolean DEFAULT true NOT NULL
);


ALTER TABLE "ViCellInstrument"."CellHealthReagents" OWNER TO "BCIViCellAdmin";


CREATE SEQUENCE "ViCellInstrument"."CellHealthReagents_IdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."CellHealthReagents_IdNum_seq" OWNER TO "BCIViCellAdmin";


ALTER SEQUENCE "ViCellInstrument"."CellHealthReagents_IdNum_seq" OWNED BY "ViCellInstrument"."CellHealthReagents"."IdNum";


--
-- TOC entry 260 (class 1259 OID 142576)
-- Name: Roles; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."Roles" (
    "RoleIdNum" bigint NOT NULL,
    "RoleID" uuid NOT NULL,
    "RoleName" character varying NOT NULL,
    "RoleType" smallint NOT NULL,
    "GroupMapList" character varying[],
    "CellTypeIndexList" integer[],
    "InstrumentPermissions" bigint,
    "ApplicationPermissions" bigint,
    "Protected" boolean DEFAULT true NOT NULL
);


ALTER TABLE "ViCellInstrument"."Roles" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 261 (class 1259 OID 142583)
-- Name: Roles_RoleIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."Roles_RoleIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."Roles_RoleIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3378 (class 0 OID 0)
-- Dependencies: 261
-- Name: Roles_RoleIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."Roles_RoleIdNum_seq" OWNED BY "ViCellInstrument"."Roles"."RoleIdNum";


--
-- TOC entry 262 (class 1259 OID 142585)
-- Name: SampleItems; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."SampleItems" (
    "SampleItemIdNum" bigint NOT NULL,
    "SampleItemID" uuid NOT NULL,
    "SampleItemStatus" smallint,
    "SampleItemName" character varying,
    "Comments" character varying,
    "RunDate" timestamp without time zone,
    "SampleSetID" uuid NOT NULL,
    "SampleID" uuid,
    "SaveImages" smallint,
    "WashType" smallint,
    "Dilution" smallint,
    "ItemLabel" character varying,
    "ImageAnalysisParamID" uuid,
    "AnalysisDefinitionID" uuid,
    "AnalysisDefinitionIndex" integer,
    "AnalysisParameterID" uuid,
    "CellTypeID" uuid,
    "CellTypeIndex" integer,
    "BioProcessID" uuid,
    "QcProcessID" uuid,
    "WorkflowID" uuid,
    "SamplePosition" character varying,
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellInstrument"."SampleItems" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 263 (class 1259 OID 142592)
-- Name: SampleItems_SampleItemIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."SampleItems_SampleItemIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."SampleItems_SampleItemIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3381 (class 0 OID 0)
-- Dependencies: 263
-- Name: SampleItems_SampleItemIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."SampleItems_SampleItemIdNum_seq" OWNED BY "ViCellInstrument"."SampleItems"."SampleItemIdNum";


--
-- TOC entry 264 (class 1259 OID 142594)
-- Name: SampleSets; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."SampleSets" (
    "SampleSetIdNum" bigint NOT NULL,
    "SampleSetID" uuid,
    "SampleSetStatus" smallint,
    "SampleSetName" character varying,
    "SampleSetLabel" character varying,
    "Comments" character varying,
    "CarrierType" smallint DEFAULT 0 NOT NULL,
    "OwnerID" uuid,
    "CreateDate" timestamp without time zone,
    "ModifyDate" timestamp without time zone,
    "RunDate" timestamp without time zone,
    "WorklistID" uuid,
    "SampleItemCount" smallint,
    "ProcessedItemCount" smallint,
    "SampleItemIDList" uuid[],
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellInstrument"."SampleSets" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 265 (class 1259 OID 142602)
-- Name: SampleSets_SampleSetIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."SampleSets_SampleSetIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."SampleSets_SampleSetIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3384 (class 0 OID 0)
-- Dependencies: 265
-- Name: SampleSets_SampleSetIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."SampleSets_SampleSetIdNum_seq" OWNED BY "ViCellInstrument"."SampleSets"."SampleSetIdNum";


--
-- TOC entry 279 (class 1259 OID 156906)
-- Name: Scheduler; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."Scheduler" (
    "SchedulerConfigIdNum" bigint NOT NULL,
    "SchedulerConfigID" uuid NOT NULL,
    "SchedulerName" character varying NOT NULL,
    "Comments" character varying,
    "OutputFilenameTemplate" character varying NOT NULL,
    "OwnerID" uuid NOT NULL,
    "CreationDate" timestamp without time zone NOT NULL,
    "OutputType" smallint DEFAULT 0 NOT NULL,
    "StartDate" timestamp without time zone,
    "StartOffset" smallint,
    "RepetitionInterval" integer DEFAULT 0 NOT NULL,
    "DayWeekIndicator" smallint DEFAULT 127 NOT NULL,
    "MonthlyRunDay" smallint DEFAULT 0 NOT NULL,
    "DestinationFolder" character varying NOT NULL,
    "DataType" integer,
    "FilterTypesList" integer[],
    "CompareOpsList" character varying[],
    "CompareValsList" character varying[],
    "Enabled" boolean DEFAULT false NOT NULL,
    "LastRunTime" timestamp without time zone,
    "LastSuccessRunTime" timestamp without time zone,
    "LastRunStatus" smallint DEFAULT 0 NOT NULL,
    "NotificationEmail" character varying,
    "EmailServer" character varying,
    "EmailServerPort" integer,
    "AuthenticateEmail" boolean,
    "EmailAccount" character varying,
    "EmailAccountAuthenticator" character varying,
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellInstrument"."Scheduler" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 278 (class 1259 OID 156904)
-- Name: Scheduler_SchedulerConfigIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."Scheduler_SchedulerConfigIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."Scheduler_SchedulerConfigIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3387 (class 0 OID 0)
-- Dependencies: 278
-- Name: Scheduler_SchedulerConfigIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."Scheduler_SchedulerConfigIdNum_seq" OWNED BY "ViCellInstrument"."Scheduler"."SchedulerConfigIdNum";


--
-- TOC entry 266 (class 1259 OID 142604)
-- Name: SignatureDefinitions; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."SignatureDefinitions" (
    "SignatureDefIdNum" bigint NOT NULL,
    "SignatureDefID" uuid NOT NULL,
    "ShortSignature" character varying,
    "ShortSignatureHash" character varying,
    "LongSignature" character varying,
    "LongSignatureHash" character varying,
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellInstrument"."SignatureDefinitions" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 267 (class 1259 OID 142611)
-- Name: SignatureDefinitions_SignatureDefIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."SignatureDefinitions_SignatureDefIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."SignatureDefinitions_SignatureDefIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3389 (class 0 OID 0)
-- Dependencies: 267
-- Name: SignatureDefinitions_SignatureDefIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."SignatureDefinitions_SignatureDefIdNum_seq" OWNED BY "ViCellInstrument"."SignatureDefinitions"."SignatureDefIdNum";


--
-- TOC entry 268 (class 1259 OID 142613)
-- Name: SystemLogs; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."SystemLogs" (
    "EntryIdNum" bigint NOT NULL,
    "EntryType" smallint NOT NULL,
    "EntryDate" timestamp without time zone NOT NULL,
    "EntryText" character varying NOT NULL,
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellInstrument"."SystemLogs" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 269 (class 1259 OID 142620)
-- Name: SystemLogs_EntryIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."SystemLogs_EntryIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."SystemLogs_EntryIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3392 (class 0 OID 0)
-- Dependencies: 269
-- Name: SystemLogs_EntryIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."SystemLogs_EntryIdNum_seq" OWNED BY "ViCellInstrument"."SystemLogs"."EntryIdNum";


--
-- TOC entry 270 (class 1259 OID 142622)
-- Name: UserProperties; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."UserProperties" (
    "PropertyIdNum" bigint NOT NULL,
    "PropertyIndex" smallint NOT NULL,
    "PropertyName" character varying NOT NULL,
    "PropertyType" smallint NOT NULL,
    "Protected" boolean DEFAULT true NOT NULL
);


ALTER TABLE "ViCellInstrument"."UserProperties" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 271 (class 1259 OID 142629)
-- Name: UserProperties_PropertyIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."UserProperties_PropertyIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."UserProperties_PropertyIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3395 (class 0 OID 0)
-- Dependencies: 271
-- Name: UserProperties_PropertyIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."UserProperties_PropertyIdNum_seq" OWNED BY "ViCellInstrument"."UserProperties"."PropertyIdNum";


--
-- TOC entry 272 (class 1259 OID 142631)
-- Name: Users; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."Users" (
    "UserIdNum" bigint NOT NULL,
    "UserID" uuid NOT NULL,
    "Retired" boolean DEFAULT false NOT NULL,
    "ADUser" boolean DEFAULT false NOT NULL,
    "RoleID" uuid NOT NULL,
    "UserName" character varying NOT NULL,
    "DisplayName" character varying,
    "Comments" character varying,
    "UserEmail" character varying,
    "AuthenticatorList" character varying[],
    "AuthenticatorDate" timestamp without time zone,
    "LastLogin" timestamp without time zone,
    "AttemptCount" smallint,
    "LanguageCode" character varying,
    "DefaultSampleName" character varying,
    "SaveNthIImage" smallint,
    "DisplayColumns" public.column_display_info[],
    "DecimalPrecision" smallint,
    "ExportFolder" character varying,
    "DefaultResultFileName" character varying,
    "CSVFolder" character varying,
    "PdfExport" boolean DEFAULT false NOT NULL,
    "AllowFastMode" boolean DEFAULT true NOT NULL,
    "WashType" smallint,
    "Dilution" smallint,
    "DefaultCellTypeIndex" integer,
    "NumUserCellTypes" smallint,
    "UserCellTypeIndexList" integer[],
    "UserAnalysisDefIndexList" integer[],
    "NumUserProperties" smallint,
    "UserPropertiesIndexList" smallint[],
    "AppPermissions" bigint,
    "AppPermissionsHash" character varying,
    "InstrumentPermissions" bigint,
    "InstrumentPermissionsHash" character varying,
    "Protected" boolean DEFAULT true NOT NULL
);


ALTER TABLE "ViCellInstrument"."Users" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 273 (class 1259 OID 142642)
-- Name: Users_UserIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."Users_UserIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."Users_UserIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3398 (class 0 OID 0)
-- Dependencies: 273
-- Name: Users_UserIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."Users_UserIdNum_seq" OWNED BY "ViCellInstrument"."Users"."UserIdNum";


--
-- TOC entry 274 (class 1259 OID 142644)
-- Name: Workflows; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."Workflows" (
    "WorkflowIdNum" bigint NOT NULL,
    "WorkflowID" uuid NOT NULL,
    "WorkflowName" character varying NOT NULL,
    "ReagentTypeList" integer[],
    "WorkflowSequenceControl" character varying NOT NULL,
    "Protected" boolean DEFAULT true NOT NULL
);


ALTER TABLE "ViCellInstrument"."Workflows" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 275 (class 1259 OID 142651)
-- Name: Workflows_WorkflowIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."Workflows_WorkflowIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."Workflows_WorkflowIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3401 (class 0 OID 0)
-- Dependencies: 275
-- Name: Workflows_WorkflowIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."Workflows_WorkflowIdNum_seq" OWNED BY "ViCellInstrument"."Workflows"."WorkflowIdNum";


--
-- TOC entry 276 (class 1259 OID 142653)
-- Name: Worklists; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."Worklists" (
    "WorklistIdNum" bigint NOT NULL,
    "WorklistID" uuid NOT NULL,
    "WorklistStatus" smallint,
    "WorklistName" character varying,
    "ListComments" character varying,
    "InstrumentSN" character varying,
    "CreationUserID" uuid NOT NULL,
    "RunUserID" uuid,
    "RunDate" timestamp without time zone,
    "AcquireSample" boolean,
    "CarrierType" smallint DEFAULT 0 NOT NULL,
    "ByColumn" boolean,
    "SaveImages" smallint NOT NULL,
    "WashType" smallint,
    "Dilution" smallint,
    "DefaultSetName" character varying,
    "DefaultItemName" character varying,
    "ImageAnalysisParamID" uuid,
    "AnalysisDefinitionID" uuid,
    "AnalysisDefinitionIndex" integer,
    "AnalysisParameterID" uuid,
    "CellTypeID" uuid,
    "CellTypeIndex" integer,
    "BioProcessID" uuid,
    "QcProcessID" uuid,
    "WorkflowID" uuid,
    "SampleSetCount" smallint,
    "ProcessedSetCount" smallint,
    "SampleSetIDList" uuid[],
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellInstrument"."Worklists" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 277 (class 1259 OID 142661)
-- Name: Worklists_WorklistIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."Worklists_WorklistIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."Worklists_WorklistIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3404 (class 0 OID 0)
-- Dependencies: 277
-- Name: Worklists_WorklistIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."Worklists_WorklistIdNum_seq" OWNED BY "ViCellInstrument"."Worklists"."WorklistIdNum";


--
-- TOC entry 2962 (class 2604 OID 142663)
-- Name: Analyses AnalysisIdNum; Type: DEFAULT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."Analyses" ALTER COLUMN "AnalysisIdNum" SET DEFAULT nextval('"ViCellData"."Analyses_AnalysisIdNum_seq"'::regclass);


--
-- TOC entry 2964 (class 2604 OID 142664)
-- Name: DetailedResults DetailedResultIdNum; Type: DEFAULT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."DetailedResults" ALTER COLUMN "DetailedResultIdNum" SET DEFAULT nextval('"ViCellData"."DetailedResults_DetailedResultIdNum_seq"'::regclass);


--
-- TOC entry 2966 (class 2604 OID 142665)
-- Name: ImageReferences ImageIdNum; Type: DEFAULT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."ImageReferences" ALTER COLUMN "ImageIdNum" SET DEFAULT nextval('"ViCellData"."ImageReferences_ImageIdNum_seq"'::regclass);


--
-- TOC entry 2968 (class 2604 OID 142666)
-- Name: ImageResults ResultIdNum; Type: DEFAULT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."ImageResults" ALTER COLUMN "ResultIdNum" SET DEFAULT nextval('"ViCellData"."ImageResults_ResultIdNum_seq"'::regclass);


--
-- TOC entry 2970 (class 2604 OID 142667)
-- Name: ImageSequences ImageSequenceIdNum; Type: DEFAULT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."ImageSequences" ALTER COLUMN "ImageSequenceIdNum" SET DEFAULT nextval('"ViCellData"."ImageSequences_ImageSequenceIdNum_seq"'::regclass);


--
-- TOC entry 2972 (class 2604 OID 142668)
-- Name: ImageSets ImageSetIdNum; Type: DEFAULT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."ImageSets" ALTER COLUMN "ImageSetIdNum" SET DEFAULT nextval('"ViCellData"."ImageSets_ImageSetIdNum_seq"'::regclass);


--
-- TOC entry 2974 (class 2604 OID 142669)
-- Name: SResults SResultIdNum; Type: DEFAULT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."SResults" ALTER COLUMN "SResultIdNum" SET DEFAULT nextval('"ViCellData"."SResults_SResultIdNum_seq"'::regclass);


--
-- TOC entry 2976 (class 2604 OID 142670)
-- Name: SampleProperties SampleIdNum; Type: DEFAULT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."SampleProperties" ALTER COLUMN "SampleIdNum" SET DEFAULT nextval('"ViCellData"."SampleProperties_SampleIdNum_seq"'::regclass);


--
-- TOC entry 2978 (class 2604 OID 142671)
-- Name: SummaryResults SummaryResultIdNum; Type: DEFAULT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."SummaryResults" ALTER COLUMN "SummaryResultIdNum" SET DEFAULT nextval('"ViCellData"."SummaryResults_SummaryResultIdNum_seq"'::regclass);


--
-- TOC entry 2981 (class 2604 OID 142672)
-- Name: AnalysisDefinitions AnalysisDefinitionIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."AnalysisDefinitions" ALTER COLUMN "AnalysisDefinitionIdNum" SET DEFAULT nextval('"ViCellInstrument"."AnalysisDefinitions_AnalysisDefinitionIdNum_seq"'::regclass);


--
-- TOC entry 2982 (class 2604 OID 142673)
-- Name: AnalysisInputSettings SettingsIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."AnalysisInputSettings" ALTER COLUMN "SettingsIdNum" SET DEFAULT nextval('"ViCellInstrument"."AnalysisInputSettings_SettingsIdNum_seq"'::regclass);


--
-- TOC entry 2984 (class 2604 OID 142674)
-- Name: AnalysisParams AnalysisParamIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."AnalysisParams" ALTER COLUMN "AnalysisParamIdNum" SET DEFAULT nextval('"ViCellInstrument"."AnalysisParams_AnalysisParamIdNum_seq"'::regclass);


--
-- TOC entry 2986 (class 2604 OID 142675)
-- Name: BioProcesses BioProcessIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."BioProcesses" ALTER COLUMN "BioProcessIdNum" SET DEFAULT nextval('"ViCellInstrument"."BioProcesses_BioProcessIdNum_seq"'::regclass);


--
-- TOC entry 2988 (class 2604 OID 142676)
-- Name: Calibrations CalibrationIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."Calibrations" ALTER COLUMN "CalibrationIdNum" SET DEFAULT nextval('"ViCellInstrument"."Calibrations_CalibrationIdNum_seq"'::regclass);


--
-- TOC entry 2992 (class 2604 OID 142677)
-- Name: CellTypes CellTypeIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."CellTypes" ALTER COLUMN "CellTypeIdNum" SET DEFAULT nextval('"ViCellInstrument"."CellTypes_CellTypeIdNum_seq"'::regclass);


--
-- TOC entry 2999 (class 2604 OID 142678)
-- Name: IlluminatorTypes IlluminatorIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."IlluminatorTypes" ALTER COLUMN "IlluminatorIdNum" SET DEFAULT nextval('"ViCellInstrument"."IlluminatorTypes_IlluminatorIdNum_seq"'::regclass);


--
-- TOC entry 3001 (class 2604 OID 142679)
-- Name: ImageAnalysisCellIdentParams IdentParamIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."ImageAnalysisCellIdentParams" ALTER COLUMN "IdentParamIdNum" SET DEFAULT nextval('"ViCellInstrument"."ImageAnalysisCellIdentParams_IdentParamIdNum_seq"'::regclass);


--
-- TOC entry 3003 (class 2604 OID 142680)
-- Name: ImageAnalysisParams ImageAnalysisParamIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."ImageAnalysisParams" ALTER COLUMN "ImageAnalysisParamIdNum" SET DEFAULT nextval('"ViCellInstrument"."ImageAnalysisParams_ImageAnalysisParamIdNum_seq"'::regclass);


--
-- TOC entry 3020 (class 2604 OID 142681)
-- Name: InstrumentConfig InstrumentIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."InstrumentConfig" ALTER COLUMN "InstrumentIdNum" SET DEFAULT nextval('"ViCellInstrument"."InstrumentConfig_InstrumentIdNum_seq"'::regclass);


--
-- TOC entry 3022 (class 2604 OID 142682)
-- Name: QcProcesses QcIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."QcProcesses" ALTER COLUMN "QcIdNum" SET DEFAULT nextval('"ViCellInstrument"."QcProcesses_QcIdNum_seq"'::regclass);


--
-- TOC entry 3024 (class 2604 OID 142683)
-- Name: ReagentInfo ReagentIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."ReagentInfo" ALTER COLUMN "ReagentIdNum" SET DEFAULT nextval('"ViCellInstrument"."ReagentInfo_ReagentIdNum_seq"'::regclass);


--


ALTER TABLE ONLY "ViCellInstrument"."CellHealthReagents" ALTER COLUMN "IdNum" SET DEFAULT nextval('"ViCellInstrument"."CellHealthReagents_IdNum_seq"'::regclass);


--
-- TOC entry 3026 (class 2604 OID 142684)
-- Name: Roles RoleIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."Roles" ALTER COLUMN "RoleIdNum" SET DEFAULT nextval('"ViCellInstrument"."Roles_RoleIdNum_seq"'::regclass);


--
-- TOC entry 3028 (class 2604 OID 142685)
-- Name: SampleItems SampleItemIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."SampleItems" ALTER COLUMN "SampleItemIdNum" SET DEFAULT nextval('"ViCellInstrument"."SampleItems_SampleItemIdNum_seq"'::regclass);


--
-- TOC entry 3031 (class 2604 OID 142686)
-- Name: SampleSets SampleSetIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."SampleSets" ALTER COLUMN "SampleSetIdNum" SET DEFAULT nextval('"ViCellInstrument"."SampleSets_SampleSetIdNum_seq"'::regclass);


--
-- TOC entry 3049 (class 2604 OID 156909)
-- Name: Scheduler SchedulerConfigIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."Scheduler" ALTER COLUMN "SchedulerConfigIdNum" SET DEFAULT nextval('"ViCellInstrument"."Scheduler_SchedulerConfigIdNum_seq"'::regclass);


--
-- TOC entry 3033 (class 2604 OID 142687)
-- Name: SignatureDefinitions SignatureDefIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."SignatureDefinitions" ALTER COLUMN "SignatureDefIdNum" SET DEFAULT nextval('"ViCellInstrument"."SignatureDefinitions_SignatureDefIdNum_seq"'::regclass);


--
-- TOC entry 3035 (class 2604 OID 142688)
-- Name: SystemLogs EntryIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."SystemLogs" ALTER COLUMN "EntryIdNum" SET DEFAULT nextval('"ViCellInstrument"."SystemLogs_EntryIdNum_seq"'::regclass);


--
-- TOC entry 3037 (class 2604 OID 142689)
-- Name: UserProperties PropertyIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."UserProperties" ALTER COLUMN "PropertyIdNum" SET DEFAULT nextval('"ViCellInstrument"."UserProperties_PropertyIdNum_seq"'::regclass);


--
-- TOC entry 3043 (class 2604 OID 142690)
-- Name: Users UserIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."Users" ALTER COLUMN "UserIdNum" SET DEFAULT nextval('"ViCellInstrument"."Users_UserIdNum_seq"'::regclass);


--
-- TOC entry 3045 (class 2604 OID 142691)
-- Name: Workflows WorkflowIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."Workflows" ALTER COLUMN "WorkflowIdNum" SET DEFAULT nextval('"ViCellInstrument"."Workflows_WorkflowIdNum_seq"'::regclass);


--
-- TOC entry 3048 (class 2604 OID 142692)
-- Name: Worklists WorklistIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."Worklists" ALTER COLUMN "WorklistIdNum" SET DEFAULT nextval('"ViCellInstrument"."Worklists_WorklistIdNum_seq"'::regclass);


--
-- TOC entry 3238 (class 0 OID 142374)
-- Dependencies: 218
-- Data for Name: Analyses; Type: TABLE DATA; Schema: ViCellData; Owner: BCIViCellAdmin
--



--
-- TOC entry 3240 (class 0 OID 142383)
-- Dependencies: 220
-- Data for Name: DetailedResults; Type: TABLE DATA; Schema: ViCellData; Owner: BCIViCellAdmin
--



--
-- TOC entry 3242 (class 0 OID 142389)
-- Dependencies: 222
-- Data for Name: ImageReferences; Type: TABLE DATA; Schema: ViCellData; Owner: BCIViCellAdmin
--



--
-- TOC entry 3244 (class 0 OID 142398)
-- Dependencies: 224
-- Data for Name: ImageResults; Type: TABLE DATA; Schema: ViCellData; Owner: BCIViCellAdmin
--



--
-- TOC entry 3246 (class 0 OID 142407)
-- Dependencies: 226
-- Data for Name: ImageSequences; Type: TABLE DATA; Schema: ViCellData; Owner: BCIViCellAdmin
--



--
-- TOC entry 3248 (class 0 OID 142416)
-- Dependencies: 228
-- Data for Name: ImageSets; Type: TABLE DATA; Schema: ViCellData; Owner: BCIViCellAdmin
--



--
-- TOC entry 3250 (class 0 OID 142425)
-- Dependencies: 230
-- Data for Name: SResults; Type: TABLE DATA; Schema: ViCellData; Owner: BCIViCellAdmin
--



--
-- TOC entry 3252 (class 0 OID 142434)
-- Dependencies: 232
-- Data for Name: SampleProperties; Type: TABLE DATA; Schema: ViCellData; Owner: BCIViCellAdmin
--



--
-- TOC entry 3254 (class 0 OID 142443)
-- Dependencies: 234
-- Data for Name: SummaryResults; Type: TABLE DATA; Schema: ViCellData; Owner: BCIViCellAdmin
--



--
-- TOC entry 3256 (class 0 OID 142452)
-- Dependencies: 236
-- Data for Name: AnalysisDefinitions; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

INSERT INTO "ViCellInstrument"."AnalysisDefinitions" ("AnalysisDefinitionIdNum", "AnalysisDefinitionID", "AnalysisDefinitionIndex", "AnalysisDefinitionName", "NumReagents", "ReagentTypeIndexList", "MixingCycles", "NumIlluminators", "IlluminatorsIndexList", "NumAnalysisParams", "AnalysisParamIDList", "PopulationParamExists", "PopulationParamID", "Protected") VALUES (1, '685c3b26-807f-4303-a890-ec86f6be6f7a', 0, 'Viable (TB)', 1, '{4}', 3, 0, '{}', 2, '{eea18195-2002-4e53-bfb7-f4ccbed6b2c7,ab64c2e2-a4b4-47b3-b40b-551f80f9b1b1}', false, NULL, true);
INSERT INTO "ViCellInstrument"."AnalysisDefinitions" ("AnalysisDefinitionIdNum", "AnalysisDefinitionID", "AnalysisDefinitionIndex", "AnalysisDefinitionName", "NumReagents", "ReagentTypeIndexList", "MixingCycles", "NumIlluminators", "IlluminatorsIndexList", "NumAnalysisParams", "AnalysisParamIDList", "PopulationParamExists", "PopulationParamID", "Protected") VALUES (2, '17615cb6-a557-4cda-88fd-0452421daa03', 0, 'Viable (TB)', 1, '{4}', 3, 0, '{}', 2, '{8ca1d65f-cdba-48e5-9a15-e0ab99a55fc9,db37a5b9-c8c3-44ca-b840-1913bb9e268e}', false, NULL, true);
INSERT INTO "ViCellInstrument"."AnalysisDefinitions" ("AnalysisDefinitionIdNum", "AnalysisDefinitionID", "AnalysisDefinitionIndex", "AnalysisDefinitionName", "NumReagents", "ReagentTypeIndexList", "MixingCycles", "NumIlluminators", "IlluminatorsIndexList", "NumAnalysisParams", "AnalysisParamIDList", "PopulationParamExists", "PopulationParamID", "Protected") VALUES (3, 'af21144c-b7ed-4445-b709-ecfad10ba38f', 0, 'Viable (TB)', 1, '{4}', 3, 0, '{}', 2, '{e259149a-b3f3-4365-8920-902ca9a39c67,0e4399bd-c7ff-4c03-8ae7-a3c27e0a3221}', false, NULL, true);
INSERT INTO "ViCellInstrument"."AnalysisDefinitions" ("AnalysisDefinitionIdNum", "AnalysisDefinitionID", "AnalysisDefinitionIndex", "AnalysisDefinitionName", "NumReagents", "ReagentTypeIndexList", "MixingCycles", "NumIlluminators", "IlluminatorsIndexList", "NumAnalysisParams", "AnalysisParamIDList", "PopulationParamExists", "PopulationParamID", "Protected") VALUES (4, '3f4d3a35-20bd-41ad-999a-beec31095464', 0, 'Viable (TB)', 1, '{4}', 3, 0, '{}', 2, '{cec1d6e2-ed5b-40d2-96e7-b78052417792,0c0589d1-3d0c-4a69-bac5-36b1e9b91ba9}', false, NULL, true);
INSERT INTO "ViCellInstrument"."AnalysisDefinitions" ("AnalysisDefinitionIdNum", "AnalysisDefinitionID", "AnalysisDefinitionIndex", "AnalysisDefinitionName", "NumReagents", "ReagentTypeIndexList", "MixingCycles", "NumIlluminators", "IlluminatorsIndexList", "NumAnalysisParams", "AnalysisParamIDList", "PopulationParamExists", "PopulationParamID", "Protected") VALUES (5, 'fba1bfb9-a30b-4a98-9571-0fe2ca5053a7', 0, 'Viable (TB)', 1, '{4}', 3, 0, '{}', 2, '{fc2bdb46-c148-4687-8d51-88a5f1c1575b,bbcb75ad-48d3-4ad5-b2b7-ac795d5e7f5d}', false, NULL, true);
INSERT INTO "ViCellInstrument"."AnalysisDefinitions" ("AnalysisDefinitionIdNum", "AnalysisDefinitionID", "AnalysisDefinitionIndex", "AnalysisDefinitionName", "NumReagents", "ReagentTypeIndexList", "MixingCycles", "NumIlluminators", "IlluminatorsIndexList", "NumAnalysisParams", "AnalysisParamIDList", "PopulationParamExists", "PopulationParamID", "Protected") VALUES (6, 'f36b4017-0f6c-4a59-afff-f21b912d40ac', 0, 'Viable (TB)', 1, '{4}', 3, 0, '{}', 2, '{639a2281-77f3-4b5b-97b6-5f0237d138d5,22b1866b-bc0f-49af-92e9-7e948a56106e}', false, NULL, true);
INSERT INTO "ViCellInstrument"."AnalysisDefinitions" ("AnalysisDefinitionIdNum", "AnalysisDefinitionID", "AnalysisDefinitionIndex", "AnalysisDefinitionName", "NumReagents", "ReagentTypeIndexList", "MixingCycles", "NumIlluminators", "IlluminatorsIndexList", "NumAnalysisParams", "AnalysisParamIDList", "PopulationParamExists", "PopulationParamID", "Protected") VALUES (7, '9f0c7de5-489c-4c0f-8747-56846cabb9d8', 0, 'Viable (TB)', 1, '{4}', 3, 0, '{}', 2, '{ced3c4b5-4248-4d1f-95b7-fb2d247002da,23a4929f-1835-4b21-a4b4-ae060b0b684e}', false, NULL, true);
INSERT INTO "ViCellInstrument"."AnalysisDefinitions" ("AnalysisDefinitionIdNum", "AnalysisDefinitionID", "AnalysisDefinitionIndex", "AnalysisDefinitionName", "NumReagents", "ReagentTypeIndexList", "MixingCycles", "NumIlluminators", "IlluminatorsIndexList", "NumAnalysisParams", "AnalysisParamIDList", "PopulationParamExists", "PopulationParamID", "Protected") VALUES (8, '4672bd76-a7ec-4a38-9457-72193392e923', 0, 'Viable (TB)', 1, '{4}', 3, 0, '{}', 2, '{27532029-f12c-4b49-99b0-25c504edaa0c,1fa1368f-d6b6-4281-92ad-ee3c39920603}', false, NULL, true);


--
-- TOC entry 3258 (class 0 OID 142462)
-- Dependencies: 238
-- Data for Name: AnalysisInputSettings; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

INSERT INTO "ViCellInstrument"."AnalysisInputSettings" ("SettingsIdNum", "SettingsID", "InputConfigParamMap", "CellIdentParamList", "POIIdentParamList") VALUES (1, 'e0e448df-94c5-421f-8224-17a7f2e6b053', '{"(0,1)","(1,1)","(2,1)","(3,0)","(4,1)","(5,0.53000000000000003)","(6,-5.5300000000000002)","(8,60)","(9,60)","(10,16)","(11,18)","(13,368)","(14,0)","(15,100)","(16,5)","(17,30)","(18,30)","(19,53)","(20,8)","(21,50)","(22,50)","(23,1)","(24,0)","(25,1)","(26,19)"}', '{"(8,0,0,8,1)","(8,0,0,50,0)","(9,0,0,0.100000001,1)","(10,0,0,7,1)"}', '{"(20,0,0,5,1)","(21,0,0,50,1)"}');
INSERT INTO "ViCellInstrument"."AnalysisInputSettings" ("SettingsIdNum", "SettingsID", "InputConfigParamMap", "CellIdentParamList", "POIIdentParamList") VALUES (2, '20a9f393-1f5c-440f-ba33-011ff47ef450', '{"(0,1)","(1,1)","(2,1)","(3,0)","(4,1)","(5,0.53000000000000003)","(6,-5.5300000000000002)","(8,60)","(9,60)","(10,16)","(11,18)","(13,368)","(14,0)","(15,100)","(16,5)","(17,30)","(18,30)","(19,53)","(20,8)","(21,50)","(22,50)","(23,1)","(24,0)","(25,1)","(26,19)"}', '{"(8,0,0,8,1)","(8,0,0,50,0)","(9,0,0,0.100000001,1)","(10,0,0,7,1)"}', '{"(20,0,0,5,1)","(21,0,0,55,1)"}');


--
-- TOC entry 3260 (class 0 OID 142470)
-- Dependencies: 240
-- Data for Name: AnalysisParams; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

INSERT INTO "ViCellInstrument"."AnalysisParams" ("AnalysisParamIdNum", "AnalysisParamID", "IsInitialized", "AnalysisParamLabel", "CharacteristicKey", "CharacteristicSKey", "CharacteristicSSKey", "ThresholdValue", "AboveThreshold", "Protected") VALUES (1, 'eea18195-2002-4e53-bfb7-f4ccbed6b2c7', false, 'Cell Spot Area', 20, 0, 0, 5, true, true);
INSERT INTO "ViCellInstrument"."AnalysisParams" ("AnalysisParamIdNum", "AnalysisParamID", "IsInitialized", "AnalysisParamLabel", "CharacteristicKey", "CharacteristicSKey", "CharacteristicSSKey", "ThresholdValue", "AboveThreshold", "Protected") VALUES (2, 'ab64c2e2-a4b4-47b3-b40b-551f80f9b1b1', false, 'Average Spot Brightness', 21, 0, 0, 50, true, true);
INSERT INTO "ViCellInstrument"."AnalysisParams" ("AnalysisParamIdNum", "AnalysisParamID", "IsInitialized", "AnalysisParamLabel", "CharacteristicKey", "CharacteristicSKey", "CharacteristicSSKey", "ThresholdValue", "AboveThreshold", "Protected") VALUES (3, '8ca1d65f-cdba-48e5-9a15-e0ab99a55fc9', false, 'Cell Spot Area', 20, 0, 0, 1, true, true);
INSERT INTO "ViCellInstrument"."AnalysisParams" ("AnalysisParamIdNum", "AnalysisParamID", "IsInitialized", "AnalysisParamLabel", "CharacteristicKey", "CharacteristicSKey", "CharacteristicSSKey", "ThresholdValue", "AboveThreshold", "Protected") VALUES (4, 'db37a5b9-c8c3-44ca-b840-1913bb9e268e', false, 'Average Spot Brightness', 21, 0, 0, 50, true, true);
INSERT INTO "ViCellInstrument"."AnalysisParams" ("AnalysisParamIdNum", "AnalysisParamID", "IsInitialized", "AnalysisParamLabel", "CharacteristicKey", "CharacteristicSKey", "CharacteristicSSKey", "ThresholdValue", "AboveThreshold", "Protected") VALUES (5, 'e259149a-b3f3-4365-8920-902ca9a39c67', false, 'Cell Spot Area', 20, 0, 0, 5, true, true);
INSERT INTO "ViCellInstrument"."AnalysisParams" ("AnalysisParamIdNum", "AnalysisParamID", "IsInitialized", "AnalysisParamLabel", "CharacteristicKey", "CharacteristicSKey", "CharacteristicSSKey", "ThresholdValue", "AboveThreshold", "Protected") VALUES (6, '0e4399bd-c7ff-4c03-8ae7-a3c27e0a3221', false, 'Average Spot Brightness', 21, 0, 0, 55, true, true);
INSERT INTO "ViCellInstrument"."AnalysisParams" ("AnalysisParamIdNum", "AnalysisParamID", "IsInitialized", "AnalysisParamLabel", "CharacteristicKey", "CharacteristicSKey", "CharacteristicSSKey", "ThresholdValue", "AboveThreshold", "Protected") VALUES (7, 'cec1d6e2-ed5b-40d2-96e7-b78052417792', false, 'Cell Spot Area', 20, 0, 0, 5, true, true);
INSERT INTO "ViCellInstrument"."AnalysisParams" ("AnalysisParamIdNum", "AnalysisParamID", "IsInitialized", "AnalysisParamLabel", "CharacteristicKey", "CharacteristicSKey", "CharacteristicSSKey", "ThresholdValue", "AboveThreshold", "Protected") VALUES (8, '0c0589d1-3d0c-4a69-bac5-36b1e9b91ba9', false, 'Average Spot Brightness', 21, 0, 0, 55, true, true);
INSERT INTO "ViCellInstrument"."AnalysisParams" ("AnalysisParamIdNum", "AnalysisParamID", "IsInitialized", "AnalysisParamLabel", "CharacteristicKey", "CharacteristicSKey", "CharacteristicSSKey", "ThresholdValue", "AboveThreshold", "Protected") VALUES (9, 'fc2bdb46-c148-4687-8d51-88a5f1c1575b', false, 'Cell Spot Area', 20, 0, 0, 5, true, true);
INSERT INTO "ViCellInstrument"."AnalysisParams" ("AnalysisParamIdNum", "AnalysisParamID", "IsInitialized", "AnalysisParamLabel", "CharacteristicKey", "CharacteristicSKey", "CharacteristicSSKey", "ThresholdValue", "AboveThreshold", "Protected") VALUES (10, 'bbcb75ad-48d3-4ad5-b2b7-ac795d5e7f5d', false, 'Average Spot Brightness', 21, 0, 0, 55, true, true);
INSERT INTO "ViCellInstrument"."AnalysisParams" ("AnalysisParamIdNum", "AnalysisParamID", "IsInitialized", "AnalysisParamLabel", "CharacteristicKey", "CharacteristicSKey", "CharacteristicSSKey", "ThresholdValue", "AboveThreshold", "Protected") VALUES (11, '639a2281-77f3-4b5b-97b6-5f0237d138d5', false, 'Cell Spot Area', 20, 0, 0, 2, true, true);
INSERT INTO "ViCellInstrument"."AnalysisParams" ("AnalysisParamIdNum", "AnalysisParamID", "IsInitialized", "AnalysisParamLabel", "CharacteristicKey", "CharacteristicSKey", "CharacteristicSSKey", "ThresholdValue", "AboveThreshold", "Protected") VALUES (12, '22b1866b-bc0f-49af-92e9-7e948a56106e', false, 'Average Spot Brightness', 21, 0, 0, 45, true, true);
INSERT INTO "ViCellInstrument"."AnalysisParams" ("AnalysisParamIdNum", "AnalysisParamID", "IsInitialized", "AnalysisParamLabel", "CharacteristicKey", "CharacteristicSKey", "CharacteristicSSKey", "ThresholdValue", "AboveThreshold", "Protected") VALUES (13, 'ced3c4b5-4248-4d1f-95b7-fb2d247002da', false, 'Cell Spot Area', 20, 0, 0, 1, true, true);
INSERT INTO "ViCellInstrument"."AnalysisParams" ("AnalysisParamIdNum", "AnalysisParamID", "IsInitialized", "AnalysisParamLabel", "CharacteristicKey", "CharacteristicSKey", "CharacteristicSSKey", "ThresholdValue", "AboveThreshold", "Protected") VALUES (14, '23a4929f-1835-4b21-a4b4-ae060b0b684e', false, 'Average Spot Brightness', 21, 0, 0, 50, true, true);
INSERT INTO "ViCellInstrument"."AnalysisParams" ("AnalysisParamIdNum", "AnalysisParamID", "IsInitialized", "AnalysisParamLabel", "CharacteristicKey", "CharacteristicSKey", "CharacteristicSSKey", "ThresholdValue", "AboveThreshold", "Protected") VALUES (15, '27532029-f12c-4b49-99b0-25c504edaa0c', false, 'Cell Spot Area', 20, 0, 0, 1, true, true);
INSERT INTO "ViCellInstrument"."AnalysisParams" ("AnalysisParamIdNum", "AnalysisParamID", "IsInitialized", "AnalysisParamLabel", "CharacteristicKey", "CharacteristicSKey", "CharacteristicSSKey", "ThresholdValue", "AboveThreshold", "Protected") VALUES (16, '1fa1368f-d6b6-4281-92ad-ee3c39920603', false, 'Average Spot Brightness', 21, 0, 0, 50, true, true);


--
-- TOC entry 3262 (class 0 OID 142479)
-- Dependencies: 242
-- Data for Name: BioProcesses; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--



--
-- TOC entry 3264 (class 0 OID 142488)
-- Dependencies: 244
-- Data for Name: Calibrations; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

INSERT INTO "ViCellInstrument"."Calibrations" ("CalibrationIdNum", "CalibrationID", "InstrumentSN", "CalibrationDate", "CalibrationUserID", "CalibrationType", "Slope", "Intercept", "ImageCount", "CalQueueID", "ConsumablesList", "Protected") VALUES (1, '3e477819-a004-4ada-8137-142c5e05ccbe', '', '2018-01-01 00:00:00', '68cd72e5-1200-4f17-ab23-832b8884c69e', 1, 368, 0, 100, '00000000-0000-0000-0000-000000000000', '{"(\"Factory Default\",n/a,0,\"2100-01-01 00:00:00\",2000000)","(\"Factory Default\",n/a,0,\"2100-01-01 00:00:00\",4000000)","(\"Factory Default\",n/a,0,\"2100-01-01 00:00:00\",10000000)"}', true);
INSERT INTO "ViCellInstrument"."Calibrations" ("CalibrationIdNum", "CalibrationID", "InstrumentSN", "CalibrationDate", "CalibrationUserID", "CalibrationType", "Slope", "Intercept", "ImageCount", "CalQueueID", "ConsumablesList", "Protected") VALUES (2, '8e4456cb-77e0-4d4d-bd16-cdbd7d71b1e4', '', '2018-01-01 00:00:00', '68cd72e5-1200-4f17-ab23-832b8884c69e', 2, 0.53, -5.53, 0, '00000000-0000-0000-0000-000000000000', '{"(\"Factory Default\",n/a,0,\"2100-01-01 00:00:00\",2000000)","(\"Factory Default\",n/a,0,\"2100-01-01 00:00:00\",4000000)","(\"Factory Default\",n/a,0,\"2100-01-01 00:00:00\",10000000)"}', true);


--
-- TOC entry 3266 (class 0 OID 142497)
-- Dependencies: 246
-- Data for Name: CellTypes; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeIdNum", "CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES (1, '4a174b37-926a-462d-a165-f7bd2c70fac7', 0, 'BCI Default', false, 100, 3, 1, 50, 0.100000001, 7, 0, '{}', 2, 0, 60, 60, 1, '{af21144c-b7ed-4445-b709-ecfad10ba38f}', 0, true);
INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeIdNum", "CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES (2, 'cb57cb2f-2448-46e5-a33e-2987e3d38860', 1, 'Mammalian', false, 100, 3, 6, 30, 0.100000001, 7, 0, '{}', 2, 0, 60, 60, 1, '{3f4d3a35-20bd-41ad-999a-beec31095464}', 0, true);
INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeIdNum", "CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES (3, 'e3fe7d7c-1d09-43b1-b13f-41b5295b352a', 2, 'Insect', false, 100, 3, 8, 50, 0.100000001, 7, 0, '{}', 2, 0, 60, 60, 1, '{fba1bfb9-a30b-4a98-9571-0fe2ca5053a7}', 0, true);
INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeIdNum", "CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES (4, '450fa2e0-2077-4d3d-bb35-f85a8f60fe62', 3, 'Yeast', false, 100, 3, 3, 20, 0.100000001, 4, 0, '{}', 3, 0, 60, 60, 1, '{f36b4017-0f6c-4a59-afff-f21b912d40ac}', 0, true);
INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeIdNum", "CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES (5, 'c3b5dbbe-feae-4cc7-8a96-c01dbd691e50', 4, 'BCI Viab Beads', false, 100, 3, 5, 25, 0.5, 22, 0, '{}', 2, 0, 60, 60, 1, '{9f0c7de5-489c-4c0f-8747-56846cabb9d8}', 0, true);
INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeIdNum", "CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES (6, '568b387f-1004-477e-8267-f01329f2e780', 5, 'BCI Conc Beads', false, 100, 3, 2.5, 12, 0.75, 17, 0, '{}', 3, 0, 60, 60, 1, '{17615cb6-a557-4cda-88fd-0452421daa03}', 0, true);
INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeIdNum", "CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES (7, 'f207e97c-851a-42e1-8c29-38721cb15d90', 6, 'BCI L10 Beads', false, 100, 3, 5, 15, 0.5, 22, 0, '{}', 2, 0, 60, 60, 1, '{4672bd76-a7ec-4a38-9457-72193392e923}', 0, true);

INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeIdNum", "CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES (8, '45b743be-abe3-436e-aab1-99bee2a7d404', 7, 'BCI Default_LowCellDensity', false, 250, 3, 1, 50, 0.100000001, 7, 0, '{}', 2, 0, 60, 60, 1, '{af21144c-b7ed-4445-b709-ecfad10ba38f}', 0, true);
INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeIdNum", "CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES (9, 'cf3720e3-a6e8-4ff6-a72d-cd433b92c92a', 8, 'Mammalian_LowCellDensity', false, 250, 3, 6, 30, 0.100000001, 7, 0, '{}', 2, 0, 60, 60, 1, '{3f4d3a35-20bd-41ad-999a-beec31095464}', 0, true);
INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeIdNum", "CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES (10, '2b397692-eb04-4a21-98a2-8f9623bc011e', 9, 'Insect_LowCellDensity', false, 250, 3, 8, 50, 0.100000001, 7, 0, '{}', 2, 0, 60, 60, 1, '{fba1bfb9-a30b-4a98-9571-0fe2ca5053a7}', 0, true);
INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeIdNum", "CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES (11, '96f826c2-890d-4fe3-a5dd-7884f45214d5', 10, 'Yeast_LowCellDensity', false, 250, 3, 3, 20, 0.100000001, 4, 0, '{}', 3, 0, 60, 60, 1, '{f36b4017-0f6c-4a59-afff-f21b912d40ac}', 0, true);
INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeIdNum", "CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES (12, '2adff531-ea8c-41c8-8e89-7acd9c23692a', 11, 'BCI Viab Beads_LowCellDensity', false, 250, 3, 5, 25, 0.5, 22, 0, '{}', 2, 0, 60, 60, 1, '{9f0c7de5-489c-4c0f-8747-56846cabb9d8}', 0, true);
INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeIdNum", "CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES (13, 'c5516ff5-ad29-407c-9d1f-9a2090827374', 12, 'BCI Conc Beads_LowCellDensity', false, 250, 3, 2.5, 12, 0.75, 17, 0, '{}', 3, 0, 60, 60, 1, '{17615cb6-a557-4cda-88fd-0452421daa03}', 0, true);
INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeIdNum", "CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES (14, 'ef8d7357-9d67-4154-aa6e-5e915ed251a5', 13, 'BCI L10 Beads_LowCellDensity', false, 250, 3, 5, 15, 0.5, 22, 0, '{}', 2, 0, 60, 60, 1, '{4672bd76-a7ec-4a38-9457-72193392e923}', 0, true);


--
-- TOC entry 3268 (class 0 OID 142508)
-- Dependencies: 248
-- Data for Name: IlluminatorTypes; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

INSERT INTO "ViCellInstrument"."IlluminatorTypes" ("IlluminatorIdNum", "IlluminatorIndex", "IlluminatorType", "IlluminatorName", "PositionNum", "Tolerance", "MaxVoltage", "IlluminatorWavelength", "EmissionWavelength", "ExposureTimeMs", "PercentPower", "SimmerVoltage", "Ltcd", "Ctld", "FeedbackPhotoDiode", "Protected") VALUES (1, 0, 1, 'BRIGHTFIELD', 1, 0.100000001, 1500000, 999, 0, 0, 20, 0, 100, 100, 1, true);
INSERT INTO "ViCellInstrument"."IlluminatorTypes" ("IlluminatorIdNum", "IlluminatorIndex", "IlluminatorType", "IlluminatorName", "PositionNum", "Tolerance", "MaxVoltage", "IlluminatorWavelength", "EmissionWavelength", "ExposureTimeMs", "PercentPower", "SimmerVoltage", "Ltcd", "Ctld", "FeedbackPhotoDiode", "Protected") VALUES (2, 1, 2, 'BRIGHTFIELD', 1, 0.100000001, 1000000, 999, 0, 0, 20, 0, 700, 0, 1, true);


--
-- TOC entry 3270 (class 0 OID 142522)
-- Dependencies: 250
-- Data for Name: ImageAnalysisCellIdentParams; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--



--
-- TOC entry 3272 (class 0 OID 142528)
-- Dependencies: 252
-- Data for Name: ImageAnalysisParams; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

INSERT INTO "ViCellInstrument"."ImageAnalysisParams" ("ImageAnalysisParamIdNum", "ImageAnalysisParamID", "AlgorithmMode", "BubbleMode", "DeclusterMode", "SubPeakAnalysisMode", "DilutionFactor", "ROIXcoords", "ROIYcoords", "DeclusterAccumulatorThreshLow", "DeclusterMinDistanceThreshLow", "DeclusterAccumulatorThreshMed", "DeclusterMinDistanceThreshMed", "DeclusterAccumulatorThreshHigh", "DeclusterMinDistanceThreshHigh", "FovDepthMM", "PixelFovMM", "SizingSlope", "SizingIntercept", "ConcSlope", "ConcIntercept", "ConcImageControlCnt", "BubbleMinSpotAreaPrcnt", "BubbleMinSpotAreaBrightness", "BubbleRejectImgAreaPrcnt", "VisibleCellSpotArea", "FlScalableROI", "FLPeakPercent", "NominalBkgdLevel", "BkgdIntensityTolerance", "CenterSpotMinIntensityLimit", "PeakIntensitySelectionAreaLimit", "CellSpotBrightnessExclusionThreshold", "HotPixelEliminationMode", "ImgBotAndRightBoundaryAnnotationMode", "SmallParticleSizingCorrection", "Protected") VALUES (1, '49c3a5b7-7e73-4cb8-960c-3a9b420d04ad', 1, true, false, false, 0, 60, 60, 20, 22, 16, 18, 12, 15, 0.085999999999999993, 0.00048000000000000001, 0, 0, 0, 0, 100, 5, 30, 30, 0, 0, 0, 53, 8, 50, 50, 1, 0, 0, 19, true);


--
-- TOC entry 3274 (class 0 OID 142534)
-- Dependencies: 254
-- Data for Name: InstrumentConfig; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

INSERT INTO "ViCellInstrument"."InstrumentConfig" ("InstrumentIdNum", "InstrumentSN", "InstrumentType", "DeviceName", "UIVersion", "SoftwareVersion", "AnalysisSWVersion", "FirmwareVersion", "BrightFieldLedType", "CameraType", "CameraFWVersion", "CameraConfig", "PumpType", "PumpFWVersion", "PumpConfig", "IlluminatorsInfoList", "IlluminatorConfig", "ConfigType", "LogName", "LogMaxSize", "LogSensitivity", "MaxLogs", "AlwaysFlush", "CameraErrorLogName", "CameraErrorLogMaxSize", "StorageErrorLogName", "StorageErrorLogMaxSize", "CarouselThetaHomeOffset", "CarouselRadiusOffset", "PlateThetaHomePosOffset", "PlateThetaCalPos", "PlateRadiusCenterPos", "SaveImage", "FocusPosition", "AutoFocus", "AbiMaxImageCount", "SampleNudgeVolume", "SampleNudgeSpeed", "FlowCellDepth", "FlowCellDepthConstant", "RfidSim", "LegacyData", "CarouselSimulator", "NightlyCleanOffset", "LastNightlyClean", "SecurityMode", "InactivityTimeout", "PasswordExpiration", "NormalShutdown", "NextAnalysisDefIndex", "NextFactoryCellTypeIndex", "NextUserCellTypeIndex", "SamplesProcessed", "DiscardCapacity", "EmailServer", "ADSettings", "LanguageList", "DefaultDisplayColumns", "RunOptionDefaults", "AutomationInstalled", "AutomationEnabled", "ACupEnabled", "AutomationPort", "Protected") VALUES (1, 'InstrumentDefault', 3, 'CellHealth Science Module', '', '', '', '', 1, 1, '', '', 1, '', '', '{"(1,0)"}', '', 0, 'HawkeyeDLL.log', 10000000, 'DBG1', 25, true, 'CameraErrorLogger.log', 1000000, 'StorageErrorLogger.log', 1000000, 0, 0, 0, 0, 0, 1, 0, '(t,45000,75000,300,2000,20,0)', 10, 5, 3, 63, 83, '(t,1,C06019_ViCell_BLU_Reagent_Pak.bin,C06002_ViCell_FLR_Reagent_Pak.bin,C06001_ViCell_FLR_Reagent_Pak.bin)', false, true, 120, NULL, 1, 60, 30, true, 8, 7, 2147483648, 0, 120, '("",0,t,"","")', '("","",0,"",f)', '{"(1033,\"English (United States)\",en-US,t)","(1031,German,de-DE,f)","(3082,Spanish,es-ES,f)","(1036,French,fr-FR,f)","(1041,Japanese,ja-JP,f)","(4,\"Chinese( Simplified )\",zh-Hans,f)"}', '{"(0,2,110,t)","(1,3,40,t)","(2,4,160,t)","(3,5,70,t)","(4,6,70,t)","(5,7,70,t)","(6,8,100,t)","(7,9,60,t)","(8,10,170,t)","(9,11,70,t)","(10,12,130,t)","(11,13,160,t)"}', '(SampleSet,Sample,100,1,t,"\\\\Instrument\\\\Export",t,"\\\\Instrument\\\\Export",Summary,"\\\\Instrument\\\\Results",f,"\\\\Instrument\\\\CSV",0,1,5)', false, false, false, 0, false);


--
-- TOC entry 3276 (class 0 OID 142558)
-- Dependencies: 256
-- Data for Name: QcProcesses; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--



--
-- TOC entry 3278 (class 0 OID 142567)
-- Dependencies: 258
-- Data for Name: ReagentInfo; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--


--


INSERT INTO "ViCellInstrument"."CellHealthReagents" ("IdNum", "Type", "ID", "Name", "Volume", "Protected") VALUES (1, 1, '00000000-0000-0000-0000-000000000000', 'Trypan Blue', 0, true);
INSERT INTO "ViCellInstrument"."CellHealthReagents" ("IdNum", "Type", "ID", "Name", "Volume", "Protected") VALUES (2, 2, '00000000-0000-0000-0000-000000000000', 'Cleaning Agent', 0, true);
INSERT INTO "ViCellInstrument"."CellHealthReagents" ("IdNum", "Type", "ID", "Name", "Volume", "Protected") VALUES (3, 3, '00000000-0000-0000-0000-000000000000', 'Conditioning Agent', 0, true);
INSERT INTO "ViCellInstrument"."CellHealthReagents" ("IdNum", "Type", "ID", "Name", "Volume", "Protected") VALUES (4, 4, '00000000-0000-0000-0000-000000000000', 'Buffer Solution', 0, true);
INSERT INTO "ViCellInstrument"."CellHealthReagents" ("IdNum", "Type", "ID", "Name", "Volume", "Protected") VALUES (5, 5, '00000000-0000-0000-0000-000000000000', 'Diluent', 0, true);



--
-- TOC entry 3280 (class 0 OID 142576)
-- Dependencies: 260
-- Data for Name: Roles; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

INSERT INTO "ViCellInstrument"."Roles" ("RoleIdNum", "RoleID", "RoleName", "RoleType", "GroupMapList", "CellTypeIndexList", "InstrumentPermissions", "ApplicationPermissions", "Protected") VALUES (1, '1956600a-701f-4bf3-9f47-cfa74338cb7c', 'DefaultAdmin', 256, '{}', '{0,1,2,3,4,5,6}', -1, -1, true);
INSERT INTO "ViCellInstrument"."Roles" ("RoleIdNum", "RoleID", "RoleName", "RoleType", "GroupMapList", "CellTypeIndexList", "InstrumentPermissions", "ApplicationPermissions", "Protected") VALUES (2, 'd52c4971-0709-40fb-91f3-a505c46d29ee', 'DefaultElevated', 16, '{}', '{0,1,2,3,4}', -1, -1, true);
INSERT INTO "ViCellInstrument"."Roles" ("RoleIdNum", "RoleID", "RoleName", "RoleType", "GroupMapList", "CellTypeIndexList", "InstrumentPermissions", "ApplicationPermissions", "Protected") VALUES (3, '87bba3c9-00ea-4c95-ae33-f958af6a42db', 'DefaultUser', 1, '{}', '{0,1,2,3}', -1, -1, true);


--
-- TOC entry 3282 (class 0 OID 142585)
-- Dependencies: 262
-- Data for Name: SampleItems; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--



--
-- TOC entry 3284 (class 0 OID 142594)
-- Dependencies: 264
-- Data for Name: SampleSets; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--



--
-- TOC entry 3299 (class 0 OID 156906)
-- Dependencies: 279
-- Data for Name: Scheduler; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--



--
-- TOC entry 3286 (class 0 OID 142604)
-- Dependencies: 266
-- Data for Name: SignatureDefinitions; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

INSERT INTO "ViCellInstrument"."SignatureDefinitions" ("SignatureDefIdNum", "SignatureDefID", "ShortSignature", "ShortSignatureHash", "LongSignature", "LongSignatureHash", "Protected") VALUES (1, '1c1fc498-2556-4d82-ad2e-f17fec8738f1', 'REV', 'A6FDAB7318A0E469FDC9D3CCA3425F7A97C5200463A987B6284F95154E865D41', 'Reviewed', '085B3CE15D58FD371B34E8C740C68D2D3DFC193E24150FEA80E23FF87691F4B7', false);
INSERT INTO "ViCellInstrument"."SignatureDefinitions" ("SignatureDefIdNum", "SignatureDefID", "ShortSignature", "ShortSignatureHash", "LongSignature", "LongSignatureHash", "Protected") VALUES (2, '0a863686-c6b1-4780-bc18-5b23541c3dbf', 'APR', 'F426332AD546DFB70EC81C7CCFEA329F96C45CF49BCBD6E692B78E0203D52E8A', 'Approved', '17973463360605335781E00132F00B5683294786BA958E0FF5CC1AC54B764B3C', false);


--
-- TOC entry 3288 (class 0 OID 142613)
-- Dependencies: 268
-- Data for Name: SystemLogs; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--



--
-- TOC entry 3290 (class 0 OID 142622)
-- Dependencies: 270
-- Data for Name: UserProperties; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--



--
-- TOC entry 3292 (class 0 OID 142631)
-- Dependencies: 272
-- Data for Name: Users; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

INSERT INTO "ViCellInstrument"."Users" ("UserIdNum", "UserID", "Retired", "ADUser", "RoleID", "UserName", "DisplayName", "Comments", "UserEmail", "AuthenticatorList", "AuthenticatorDate", "LastLogin", "AttemptCount", "LanguageCode", "DefaultSampleName", "SaveNthIImage", "DisplayColumns", "DecimalPrecision", "ExportFolder", "DefaultResultFileName", "CSVFolder", "PdfExport", "AllowFastMode", "WashType", "Dilution", "DefaultCellTypeIndex", "NumUserCellTypes", "UserCellTypeIndexList", "UserAnalysisDefIndexList", "NumUserProperties", "UserPropertiesIndexList", "AppPermissions", "AppPermissionsHash", "InstrumentPermissions", "InstrumentPermissionsHash", "Protected") VALUES (1, '68cd72e5-1200-4f17-ab23-832b8884c69e', false, false, '1956600a-701f-4bf3-9f47-cfa74338cb7c', 'factory_admin', 'factory_admin', NULL, NULL, '{factory_admin:CD5D2C56C5A234EBBFFEC87E4FC4E2FFDD476ABB:1505EE967FBBC3AF6E5BC11ECA768482D11EE642DDDF2D4023BD6FE656F39B08:262889640FA4BCBC318F98C27A82189E9C5AB6AE40EB8C8CF1AA15232EF747CE:73F0867D3286442A784E2B8582557520D2114D1499C368764543172933224D9A}', '1970-01-01 00:00:00', '1970-01-01 00:00:00', 0, NULL, 'Sample', 1, '{"(0,2,110,t)","(1,3,40,t)","(2,4,160,t)","(3,5,70,t)","(4,6,70,t)","(5,7,70,t)","(6,8,100,t)","(7,9,60,t)","(8,10,170,t)","(9,11,70,t)","(10,12,130,t)","(11,13,160,t)"}', 5, '\\Instrument\\Export\\factory_admin', 'Summary', '\\Instrument\\Export\\CSV\\factory_admin', false, true, 0, 1, 0, 7, '{0,1,2,3,4,5,6}', '{0,0,0,0,0,0,0}', 0, '{}', -1, '', -1, '', true);


--
-- TOC entry 3294 (class 0 OID 142644)
-- Dependencies: 274
-- Data for Name: Workflows; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--



--
-- TOC entry 3296 (class 0 OID 142653)
-- Dependencies: 276
-- Data for Name: Worklists; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--



--
-- TOC entry 3406 (class 0 OID 0)
-- Dependencies: 219
-- Name: Analyses_AnalysisIdNum_seq; Type: SEQUENCE SET; Schema: ViCellData; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellData"."Analyses_AnalysisIdNum_seq"', 1, false);


--
-- TOC entry 3407 (class 0 OID 0)
-- Dependencies: 221
-- Name: DetailedResults_DetailedResultIdNum_seq; Type: SEQUENCE SET; Schema: ViCellData; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellData"."DetailedResults_DetailedResultIdNum_seq"', 1, false);


--
-- TOC entry 3408 (class 0 OID 0)
-- Dependencies: 223
-- Name: ImageReferences_ImageIdNum_seq; Type: SEQUENCE SET; Schema: ViCellData; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellData"."ImageReferences_ImageIdNum_seq"', 1, false);


--
-- TOC entry 3409 (class 0 OID 0)
-- Dependencies: 225
-- Name: ImageResults_ResultIdNum_seq; Type: SEQUENCE SET; Schema: ViCellData; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellData"."ImageResults_ResultIdNum_seq"', 1, false);


--
-- TOC entry 3410 (class 0 OID 0)
-- Dependencies: 227
-- Name: ImageSequences_ImageSequenceIdNum_seq; Type: SEQUENCE SET; Schema: ViCellData; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellData"."ImageSequences_ImageSequenceIdNum_seq"', 1, false);


--
-- TOC entry 3411 (class 0 OID 0)
-- Dependencies: 229
-- Name: ImageSets_ImageSetIdNum_seq; Type: SEQUENCE SET; Schema: ViCellData; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellData"."ImageSets_ImageSetIdNum_seq"', 1, false);


--
-- TOC entry 3412 (class 0 OID 0)
-- Dependencies: 231
-- Name: SResults_SResultIdNum_seq; Type: SEQUENCE SET; Schema: ViCellData; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellData"."SResults_SResultIdNum_seq"', 1, false);


--
-- TOC entry 3413 (class 0 OID 0)
-- Dependencies: 233
-- Name: SampleProperties_SampleIdNum_seq; Type: SEQUENCE SET; Schema: ViCellData; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellData"."SampleProperties_SampleIdNum_seq"', 1, false);


--
-- TOC entry 3414 (class 0 OID 0)
-- Dependencies: 235
-- Name: SummaryResults_SummaryResultIdNum_seq; Type: SEQUENCE SET; Schema: ViCellData; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellData"."SummaryResults_SummaryResultIdNum_seq"', 1, false);


--
-- TOC entry 3415 (class 0 OID 0)
-- Dependencies: 237
-- Name: AnalysisDefinitions_AnalysisDefinitionIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."AnalysisDefinitions_AnalysisDefinitionIdNum_seq"', 8, true);


--
-- TOC entry 3416 (class 0 OID 0)
-- Dependencies: 239
-- Name: AnalysisInputSettings_SettingsIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."AnalysisInputSettings_SettingsIdNum_seq"', 2, true);


--
-- TOC entry 3417 (class 0 OID 0)
-- Dependencies: 241
-- Name: AnalysisParams_AnalysisParamIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."AnalysisParams_AnalysisParamIdNum_seq"', 16, true);


--
-- TOC entry 3418 (class 0 OID 0)
-- Dependencies: 243
-- Name: BioProcesses_BioProcessIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."BioProcesses_BioProcessIdNum_seq"', 1, false);


--
-- TOC entry 3419 (class 0 OID 0)
-- Dependencies: 245
-- Name: Calibrations_CalibrationIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."Calibrations_CalibrationIdNum_seq"', 2, true);


--
-- TOC entry 3420 (class 0 OID 0)
-- Dependencies: 247
-- Name: CellTypes_CellTypeIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."CellTypes_CellTypeIdNum_seq"', 7, true);


--
-- TOC entry 3421 (class 0 OID 0)
-- Dependencies: 249
-- Name: IlluminatorTypes_IlluminatorIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."IlluminatorTypes_IlluminatorIdNum_seq"', 1, true);


--
-- TOC entry 3422 (class 0 OID 0)
-- Dependencies: 251
-- Name: ImageAnalysisCellIdentParams_IdentParamIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."ImageAnalysisCellIdentParams_IdentParamIdNum_seq"', 1, false);


--
-- TOC entry 3423 (class 0 OID 0)
-- Dependencies: 253
-- Name: ImageAnalysisParams_ImageAnalysisParamIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."ImageAnalysisParams_ImageAnalysisParamIdNum_seq"', 1, true);


--
-- TOC entry 3424 (class 0 OID 0)
-- Dependencies: 255
-- Name: InstrumentConfig_InstrumentIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."InstrumentConfig_InstrumentIdNum_seq"', 1, true);


--
-- TOC entry 3425 (class 0 OID 0)
-- Dependencies: 257
-- Name: QcProcesses_QcIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."QcProcesses_QcIdNum_seq"', 1, false);


--
-- TOC entry 3426 (class 0 OID 0)
-- Dependencies: 259
-- Name: ReagentInfo_ReagentIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."ReagentInfo_ReagentIdNum_seq"', 1, false);



SELECT pg_catalog.setval('"ViCellInstrument"."CellHealthReagents_IdNum_seq"', 1, false);


--
-- TOC entry 3427 (class 0 OID 0)
-- Dependencies: 261
-- Name: Roles_RoleIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."Roles_RoleIdNum_seq"', 3, true);


--
-- TOC entry 3428 (class 0 OID 0)
-- Dependencies: 263
-- Name: SampleItems_SampleItemIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."SampleItems_SampleItemIdNum_seq"', 1, false);


--
-- TOC entry 3429 (class 0 OID 0)
-- Dependencies: 265
-- Name: SampleSets_SampleSetIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."SampleSets_SampleSetIdNum_seq"', 1, false);


--
-- TOC entry 3430 (class 0 OID 0)
-- Dependencies: 278
-- Name: Scheduler_SchedulerConfigIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."Scheduler_SchedulerConfigIdNum_seq"', 1, false);


--
-- TOC entry 3431 (class 0 OID 0)
-- Dependencies: 267
-- Name: SignatureDefinitions_SignatureDefIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."SignatureDefinitions_SignatureDefIdNum_seq"', 2, true);


--
-- TOC entry 3432 (class 0 OID 0)
-- Dependencies: 269
-- Name: SystemLogs_EntryIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."SystemLogs_EntryIdNum_seq"', 1, false);


--
-- TOC entry 3433 (class 0 OID 0)
-- Dependencies: 271
-- Name: UserProperties_PropertyIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."UserProperties_PropertyIdNum_seq"', 1, false);


--
-- TOC entry 3434 (class 0 OID 0)
-- Dependencies: 273
-- Name: Users_UserIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."Users_UserIdNum_seq"', 1, true);


--
-- TOC entry 3435 (class 0 OID 0)
-- Dependencies: 275
-- Name: Workflows_WorkflowIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."Workflows_WorkflowIdNum_seq"', 1, false);


--
-- TOC entry 3436 (class 0 OID 0)
-- Dependencies: 277
-- Name: Worklists_WorklistIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."Worklists_WorklistIdNum_seq"', 1, false);


--
-- TOC entry 3056 (class 2606 OID 142694)
-- Name: Analyses Analyses_pkey; Type: CONSTRAINT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."Analyses"
    ADD CONSTRAINT "Analyses_pkey" PRIMARY KEY ("AnalysisID");


--
-- TOC entry 3058 (class 2606 OID 142696)
-- Name: DetailedResults DetailedResults_pkey; Type: CONSTRAINT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."DetailedResults"
    ADD CONSTRAINT "DetailedResults_pkey" PRIMARY KEY ("DetailedResultID");


--
-- TOC entry 3060 (class 2606 OID 142698)
-- Name: ImageReferences ImageReferences_pkey; Type: CONSTRAINT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."ImageReferences"
    ADD CONSTRAINT "ImageReferences_pkey" PRIMARY KEY ("ImageID");


--
-- TOC entry 3062 (class 2606 OID 142700)
-- Name: ImageResults ImageResultsMap_pkey; Type: CONSTRAINT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."ImageResults"
    ADD CONSTRAINT "ImageResultsMap_pkey" PRIMARY KEY ("ResultID");


--
-- TOC entry 3064 (class 2606 OID 142702)
-- Name: ImageSequences ImageSequences_pkey; Type: CONSTRAINT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."ImageSequences"
    ADD CONSTRAINT "ImageSequences_pkey" PRIMARY KEY ("ImageSequenceID");


--
-- TOC entry 3066 (class 2606 OID 142704)
-- Name: ImageSets ImageSets_pkey; Type: CONSTRAINT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."ImageSets"
    ADD CONSTRAINT "ImageSets_pkey" PRIMARY KEY ("ImageSetID");


--
-- TOC entry 3068 (class 2606 OID 142706)
-- Name: SResults SResults_pkey; Type: CONSTRAINT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."SResults"
    ADD CONSTRAINT "SResults_pkey" PRIMARY KEY ("SResultID");


--
-- TOC entry 3070 (class 2606 OID 142708)
-- Name: SampleProperties SampleProperties_pkey; Type: CONSTRAINT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."SampleProperties"
    ADD CONSTRAINT "SampleProperties_pkey" PRIMARY KEY ("SampleID");


--
-- TOC entry 3072 (class 2606 OID 142710)
-- Name: SummaryResults SummaryResults_pkey; Type: CONSTRAINT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."SummaryResults"
    ADD CONSTRAINT "SummaryResults_pkey" PRIMARY KEY ("SummaryResultID");


--
-- TOC entry 3074 (class 2606 OID 142712)
-- Name: AnalysisDefinitions AnalysisDefinitions_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."AnalysisDefinitions"
    ADD CONSTRAINT "AnalysisDefinitions_pkey" PRIMARY KEY ("AnalysisDefinitionID");


--
-- TOC entry 3076 (class 2606 OID 142714)
-- Name: AnalysisInputSettings AnalysisInputSettings_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."AnalysisInputSettings"
    ADD CONSTRAINT "AnalysisInputSettings_pkey" PRIMARY KEY ("SettingsID");


--
-- TOC entry 3078 (class 2606 OID 142716)
-- Name: AnalysisParams AnalysisParams_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."AnalysisParams"
    ADD CONSTRAINT "AnalysisParams_pkey" PRIMARY KEY ("AnalysisParamID");


--
-- TOC entry 3080 (class 2606 OID 142718)
-- Name: BioProcesses BioProcesses_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."BioProcesses"
    ADD CONSTRAINT "BioProcesses_pkey" PRIMARY KEY ("BioProcessID");


--
-- TOC entry 3082 (class 2606 OID 142720)
-- Name: Calibrations Calibrations_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."Calibrations"
    ADD CONSTRAINT "Calibrations_pkey" PRIMARY KEY ("CalibrationID");


--
-- TOC entry 3084 (class 2606 OID 142722)
-- Name: CellTypes CellTypes_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."CellTypes"
    ADD CONSTRAINT "CellTypes_pkey" PRIMARY KEY ("CellTypeID");


--
-- TOC entry 3086 (class 2606 OID 142724)
-- Name: IlluminatorTypes IlluminatorTypes_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."IlluminatorTypes"
    ADD CONSTRAINT "IlluminatorTypes_pkey" PRIMARY KEY ("IlluminatorIndex");


--
-- TOC entry 3088 (class 2606 OID 142726)
-- Name: ImageAnalysisCellIdentParams ImageAnalysisCellIdentParams_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."ImageAnalysisCellIdentParams"
    ADD CONSTRAINT "ImageAnalysisCellIdentParams_pkey" PRIMARY KEY ("IdentParamID");


--
-- TOC entry 3090 (class 2606 OID 142728)
-- Name: ImageAnalysisParams ImageAnalysisParams_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."ImageAnalysisParams"
    ADD CONSTRAINT "ImageAnalysisParams_pkey" PRIMARY KEY ("ImageAnalysisParamID");


--
-- TOC entry 3092 (class 2606 OID 142730)
-- Name: InstrumentConfig InstrumentConfig_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."InstrumentConfig"
    ADD CONSTRAINT "InstrumentConfig_pkey" PRIMARY KEY ("InstrumentIdNum");


--
-- TOC entry 3110 (class 2606 OID 142732)
-- Name: Users InstrumentUsers_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."Users"
    ADD CONSTRAINT "InstrumentUsers_pkey" PRIMARY KEY ("UserID");


--
-- TOC entry 3094 (class 2606 OID 142734)
-- Name: QcProcesses QcProcesses_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."QcProcesses"
    ADD CONSTRAINT "QcProcesses_pkey" PRIMARY KEY ("QcID");


--
-- TOC entry 3096 (class 2606 OID 142736)
-- Name: ReagentInfo ReagentInfo_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."ReagentInfo"
    ADD CONSTRAINT "ReagentInfo_pkey" PRIMARY KEY ("ReagentIdNum");


--


ALTER TABLE ONLY "ViCellInstrument"."CellHealthReagents"
    ADD CONSTRAINT "CellHealthReagents_pkey" PRIMARY KEY ("IdNum");


--
-- TOC entry 3098 (class 2606 OID 142738)
-- Name: Roles Roles_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."Roles"
    ADD CONSTRAINT "Roles_pkey" PRIMARY KEY ("RoleID");


--
-- TOC entry 3100 (class 2606 OID 142740)
-- Name: SampleItems SampleItems_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."SampleItems"
    ADD CONSTRAINT "SampleItems_pkey" PRIMARY KEY ("SampleItemIdNum");


--
-- TOC entry 3102 (class 2606 OID 142742)
-- Name: SampleSets SampleSets_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."SampleSets"
    ADD CONSTRAINT "SampleSets_pkey" PRIMARY KEY ("SampleSetIdNum");


--
-- TOC entry 3116 (class 2606 OID 156914)
-- Name: Scheduler Scheduler_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."Scheduler"
    ADD CONSTRAINT "Scheduler_pkey" PRIMARY KEY ("SchedulerConfigID");


--
-- TOC entry 3104 (class 2606 OID 142744)
-- Name: SignatureDefinitions SignatureDefinitions_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."SignatureDefinitions"
    ADD CONSTRAINT "SignatureDefinitions_pkey" PRIMARY KEY ("SignatureDefID");


--
-- TOC entry 3106 (class 2606 OID 142746)
-- Name: SystemLogs SystemLogs_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."SystemLogs"
    ADD CONSTRAINT "SystemLogs_pkey" PRIMARY KEY ("EntryIdNum");


--
-- TOC entry 3108 (class 2606 OID 142748)
-- Name: UserProperties UserProperties_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."UserProperties"
    ADD CONSTRAINT "UserProperties_pkey" PRIMARY KEY ("PropertyIndex");


--
-- TOC entry 3112 (class 2606 OID 142750)
-- Name: Workflows Workflows_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."Workflows"
    ADD CONSTRAINT "Workflows_pkey" PRIMARY KEY ("WorkflowID");


--
-- TOC entry 3114 (class 2606 OID 142752)
-- Name: Worklists Worklists_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."Worklists"
    ADD CONSTRAINT "Worklists_pkey" PRIMARY KEY ("WorklistIdNum");


--
-- TOC entry 3306 (class 0 OID 0)
-- Dependencies: 9
-- Name: SCHEMA "ViCellData"; Type: ACL; Schema: -; Owner: BCIViCellAdmin
--

REVOKE ALL ON SCHEMA "ViCellData" FROM "BCIViCellAdmin";
GRANT ALL ON SCHEMA "ViCellData" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SCHEMA "ViCellData" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SCHEMA "ViCellData" TO "ViCellAdmin";
GRANT ALL ON SCHEMA "ViCellData" TO "ViCellDBAdmin";
GRANT ALL ON SCHEMA "ViCellData" TO "ViCellInstrumentUser";
GRANT USAGE ON SCHEMA "ViCellData" TO "DbBackupUser";


--
-- TOC entry 3307 (class 0 OID 0)
-- Dependencies: 4
-- Name: SCHEMA "ViCellInstrument"; Type: ACL; Schema: -; Owner: BCIViCellAdmin
--

REVOKE ALL ON SCHEMA "ViCellInstrument" FROM "BCIViCellAdmin";
GRANT ALL ON SCHEMA "ViCellInstrument" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SCHEMA "ViCellInstrument" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SCHEMA "ViCellInstrument" TO "ViCellAdmin";
GRANT ALL ON SCHEMA "ViCellInstrument" TO "ViCellDBAdmin";
GRANT ALL ON SCHEMA "ViCellInstrument" TO "ViCellInstrumentUser";
GRANT USAGE ON SCHEMA "ViCellInstrument" TO "DbBackupUser";


--
-- TOC entry 3309 (class 0 OID 0)
-- Dependencies: 7
-- Name: SCHEMA public; Type: ACL; Schema: -; Owner: postgres
--

GRANT ALL ON SCHEMA public TO PUBLIC;


--
-- TOC entry 3311 (class 0 OID 0)
-- Dependencies: 218
-- Name: TABLE "Analyses"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellData"."Analyses" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."Analyses" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."Analyses" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."Analyses" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."Analyses" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellData"."Analyses" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellData"."Analyses" TO "DbBackupUser";


--
-- TOC entry 3313 (class 0 OID 0)
-- Dependencies: 219
-- Name: SEQUENCE "Analyses_AnalysisIdNum_seq"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellData"."Analyses_AnalysisIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."Analyses_AnalysisIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."Analyses_AnalysisIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."Analyses_AnalysisIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."Analyses_AnalysisIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."Analyses_AnalysisIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellData"."Analyses_AnalysisIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3314 (class 0 OID 0)
-- Dependencies: 220
-- Name: TABLE "DetailedResults"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellData"."DetailedResults" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."DetailedResults" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."DetailedResults" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."DetailedResults" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."DetailedResults" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellData"."DetailedResults" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellData"."DetailedResults" TO "DbBackupUser";


--
-- TOC entry 3316 (class 0 OID 0)
-- Dependencies: 221
-- Name: SEQUENCE "DetailedResults_DetailedResultIdNum_seq"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellData"."DetailedResults_DetailedResultIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."DetailedResults_DetailedResultIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."DetailedResults_DetailedResultIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."DetailedResults_DetailedResultIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."DetailedResults_DetailedResultIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."DetailedResults_DetailedResultIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellData"."DetailedResults_DetailedResultIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3318 (class 0 OID 0)
-- Dependencies: 222
-- Name: TABLE "ImageReferences"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellData"."ImageReferences" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."ImageReferences" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."ImageReferences" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."ImageReferences" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."ImageReferences" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellData"."ImageReferences" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellData"."ImageReferences" TO "DbBackupUser";


--
-- TOC entry 3320 (class 0 OID 0)
-- Dependencies: 223
-- Name: SEQUENCE "ImageReferences_ImageIdNum_seq"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellData"."ImageReferences_ImageIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."ImageReferences_ImageIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."ImageReferences_ImageIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."ImageReferences_ImageIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."ImageReferences_ImageIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."ImageReferences_ImageIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellData"."ImageReferences_ImageIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3321 (class 0 OID 0)
-- Dependencies: 224
-- Name: TABLE "ImageResults"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellData"."ImageResults" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."ImageResults" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."ImageResults" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."ImageResults" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."ImageResults" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellData"."ImageResults" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellData"."ImageResults" TO "DbBackupUser";


--
-- TOC entry 3323 (class 0 OID 0)
-- Dependencies: 225
-- Name: SEQUENCE "ImageResults_ResultIdNum_seq"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellData"."ImageResults_ResultIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."ImageResults_ResultIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."ImageResults_ResultIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."ImageResults_ResultIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."ImageResults_ResultIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."ImageResults_ResultIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellData"."ImageResults_ResultIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3325 (class 0 OID 0)
-- Dependencies: 226
-- Name: TABLE "ImageSequences"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellData"."ImageSequences" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."ImageSequences" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."ImageSequences" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."ImageSequences" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."ImageSequences" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellData"."ImageSequences" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellData"."ImageSequences" TO "DbBackupUser";


--
-- TOC entry 3327 (class 0 OID 0)
-- Dependencies: 227
-- Name: SEQUENCE "ImageSequences_ImageSequenceIdNum_seq"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellData"."ImageSequences_ImageSequenceIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."ImageSequences_ImageSequenceIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."ImageSequences_ImageSequenceIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."ImageSequences_ImageSequenceIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."ImageSequences_ImageSequenceIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."ImageSequences_ImageSequenceIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellData"."ImageSequences_ImageSequenceIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3329 (class 0 OID 0)
-- Dependencies: 228
-- Name: TABLE "ImageSets"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellData"."ImageSets" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."ImageSets" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."ImageSets" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."ImageSets" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."ImageSets" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellData"."ImageSets" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellData"."ImageSets" TO "DbBackupUser";


--
-- TOC entry 3331 (class 0 OID 0)
-- Dependencies: 229
-- Name: SEQUENCE "ImageSets_ImageSetIdNum_seq"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellData"."ImageSets_ImageSetIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."ImageSets_ImageSetIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."ImageSets_ImageSetIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."ImageSets_ImageSetIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."ImageSets_ImageSetIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."ImageSets_ImageSetIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellData"."ImageSets_ImageSetIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3332 (class 0 OID 0)
-- Dependencies: 230
-- Name: TABLE "SResults"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellData"."SResults" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."SResults" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."SResults" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."SResults" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."SResults" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,UPDATE ON TABLE "ViCellData"."SResults" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellData"."SResults" TO "DbBackupUser";


--
-- TOC entry 3334 (class 0 OID 0)
-- Dependencies: 231
-- Name: SEQUENCE "SResults_SResultIdNum_seq"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellData"."SResults_SResultIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."SResults_SResultIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."SResults_SResultIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."SResults_SResultIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."SResults_SResultIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."SResults_SResultIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellData"."SResults_SResultIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3335 (class 0 OID 0)
-- Dependencies: 232
-- Name: TABLE "SampleProperties"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellData"."SampleProperties" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."SampleProperties" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."SampleProperties" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."SampleProperties" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."SampleProperties" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellData"."SampleProperties" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellData"."SampleProperties" TO "DbBackupUser";


--
-- TOC entry 3337 (class 0 OID 0)
-- Dependencies: 233
-- Name: SEQUENCE "SampleProperties_SampleIdNum_seq"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellData"."SampleProperties_SampleIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."SampleProperties_SampleIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."SampleProperties_SampleIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."SampleProperties_SampleIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."SampleProperties_SampleIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."SampleProperties_SampleIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellData"."SampleProperties_SampleIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3338 (class 0 OID 0)
-- Dependencies: 234
-- Name: TABLE "SummaryResults"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellData"."SummaryResults" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."SummaryResults" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."SummaryResults" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."SummaryResults" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."SummaryResults" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellData"."SummaryResults" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellData"."SummaryResults" TO "DbBackupUser";


--
-- TOC entry 3340 (class 0 OID 0)
-- Dependencies: 235
-- Name: SEQUENCE "SummaryResults_SummaryResultIdNum_seq"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellData"."SummaryResults_SummaryResultIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."SummaryResults_SummaryResultIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."SummaryResults_SummaryResultIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."SummaryResults_SummaryResultIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."SummaryResults_SummaryResultIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."SummaryResults_SummaryResultIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellData"."SummaryResults_SummaryResultIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3341 (class 0 OID 0)
-- Dependencies: 236
-- Name: TABLE "AnalysisDefinitions"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."AnalysisDefinitions" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."AnalysisDefinitions" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."AnalysisDefinitions" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."AnalysisDefinitions" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."AnalysisDefinitions" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."AnalysisDefinitions" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."AnalysisDefinitions" TO "DbBackupUser";


--
-- TOC entry 3343 (class 0 OID 0)
-- Dependencies: 237
-- Name: SEQUENCE "AnalysisDefinitions_AnalysisDefinitionIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."AnalysisDefinitions_AnalysisDefinitionIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."AnalysisDefinitions_AnalysisDefinitionIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."AnalysisDefinitions_AnalysisDefinitionIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."AnalysisDefinitions_AnalysisDefinitionIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."AnalysisDefinitions_AnalysisDefinitionIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."AnalysisDefinitions_AnalysisDefinitionIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."AnalysisDefinitions_AnalysisDefinitionIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3344 (class 0 OID 0)
-- Dependencies: 238
-- Name: TABLE "AnalysisInputSettings"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."AnalysisInputSettings" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."AnalysisInputSettings" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."AnalysisInputSettings" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."AnalysisInputSettings" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."AnalysisInputSettings" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,UPDATE ON TABLE "ViCellInstrument"."AnalysisInputSettings" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."AnalysisInputSettings" TO "DbBackupUser";


--
-- TOC entry 3346 (class 0 OID 0)
-- Dependencies: 239
-- Name: SEQUENCE "AnalysisInputSettings_SettingsIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."AnalysisInputSettings_SettingsIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."AnalysisInputSettings_SettingsIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."AnalysisInputSettings_SettingsIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."AnalysisInputSettings_SettingsIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."AnalysisInputSettings_SettingsIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."AnalysisInputSettings_SettingsIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."AnalysisInputSettings_SettingsIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3347 (class 0 OID 0)
-- Dependencies: 240
-- Name: TABLE "AnalysisParams"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."AnalysisParams" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."AnalysisParams" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."AnalysisParams" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."AnalysisParams" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."AnalysisParams" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."AnalysisParams" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."AnalysisParams" TO "DbBackupUser";


--
-- TOC entry 3349 (class 0 OID 0)
-- Dependencies: 241
-- Name: SEQUENCE "AnalysisParams_AnalysisParamIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."AnalysisParams_AnalysisParamIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."AnalysisParams_AnalysisParamIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."AnalysisParams_AnalysisParamIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."AnalysisParams_AnalysisParamIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."AnalysisParams_AnalysisParamIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."AnalysisParams_AnalysisParamIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."AnalysisParams_AnalysisParamIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3350 (class 0 OID 0)
-- Dependencies: 242
-- Name: TABLE "BioProcesses"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."BioProcesses" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."BioProcesses" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."BioProcesses" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."BioProcesses" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."BioProcesses" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."BioProcesses" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."BioProcesses" TO "DbBackupUser";


--
-- TOC entry 3352 (class 0 OID 0)
-- Dependencies: 243
-- Name: SEQUENCE "BioProcesses_BioProcessIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."BioProcesses_BioProcessIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."BioProcesses_BioProcessIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."BioProcesses_BioProcessIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."BioProcesses_BioProcessIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."BioProcesses_BioProcessIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."BioProcesses_BioProcessIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."BioProcesses_BioProcessIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3353 (class 0 OID 0)
-- Dependencies: 244
-- Name: TABLE "Calibrations"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."Calibrations" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."Calibrations" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."Calibrations" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."Calibrations" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."Calibrations" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,UPDATE ON TABLE "ViCellInstrument"."Calibrations" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."Calibrations" TO "DbBackupUser";


--
-- TOC entry 3355 (class 0 OID 0)
-- Dependencies: 245
-- Name: SEQUENCE "Calibrations_CalibrationIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."Calibrations_CalibrationIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Calibrations_CalibrationIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."Calibrations_CalibrationIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."Calibrations_CalibrationIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Calibrations_CalibrationIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Calibrations_CalibrationIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."Calibrations_CalibrationIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3356 (class 0 OID 0)
-- Dependencies: 246
-- Name: TABLE "CellTypes"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."CellTypes" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."CellTypes" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."CellTypes" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."CellTypes" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."CellTypes" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."CellTypes" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."CellTypes" TO "DbBackupUser";


--
-- TOC entry 3358 (class 0 OID 0)
-- Dependencies: 247
-- Name: SEQUENCE "CellTypes_CellTypeIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."CellTypes_CellTypeIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."CellTypes_CellTypeIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."CellTypes_CellTypeIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."CellTypes_CellTypeIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."CellTypes_CellTypeIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."CellTypes_CellTypeIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."CellTypes_CellTypeIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3359 (class 0 OID 0)
-- Dependencies: 248
-- Name: TABLE "IlluminatorTypes"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."IlluminatorTypes" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."IlluminatorTypes" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."IlluminatorTypes" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."IlluminatorTypes" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."IlluminatorTypes" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."IlluminatorTypes" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."IlluminatorTypes" TO "DbBackupUser";


--
-- TOC entry 3361 (class 0 OID 0)
-- Dependencies: 249
-- Name: SEQUENCE "IlluminatorTypes_IlluminatorIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."IlluminatorTypes_IlluminatorIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."IlluminatorTypes_IlluminatorIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."IlluminatorTypes_IlluminatorIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."IlluminatorTypes_IlluminatorIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."IlluminatorTypes_IlluminatorIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."IlluminatorTypes_IlluminatorIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."IlluminatorTypes_IlluminatorIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3362 (class 0 OID 0)
-- Dependencies: 250
-- Name: TABLE "ImageAnalysisCellIdentParams"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."ImageAnalysisCellIdentParams" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."ImageAnalysisCellIdentParams" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."ImageAnalysisCellIdentParams" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."ImageAnalysisCellIdentParams" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."ImageAnalysisCellIdentParams" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."ImageAnalysisCellIdentParams" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."ImageAnalysisCellIdentParams" TO "DbBackupUser";


--
-- TOC entry 3364 (class 0 OID 0)
-- Dependencies: 251
-- Name: SEQUENCE "ImageAnalysisCellIdentParams_IdentParamIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."ImageAnalysisCellIdentParams_IdentParamIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."ImageAnalysisCellIdentParams_IdentParamIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."ImageAnalysisCellIdentParams_IdentParamIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."ImageAnalysisCellIdentParams_IdentParamIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."ImageAnalysisCellIdentParams_IdentParamIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."ImageAnalysisCellIdentParams_IdentParamIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."ImageAnalysisCellIdentParams_IdentParamIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3365 (class 0 OID 0)
-- Dependencies: 252
-- Name: TABLE "ImageAnalysisParams"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."ImageAnalysisParams" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."ImageAnalysisParams" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."ImageAnalysisParams" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."ImageAnalysisParams" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."ImageAnalysisParams" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."ImageAnalysisParams" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."ImageAnalysisParams" TO "DbBackupUser";


--
-- TOC entry 3367 (class 0 OID 0)
-- Dependencies: 253
-- Name: SEQUENCE "ImageAnalysisParams_ImageAnalysisParamIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."ImageAnalysisParams_ImageAnalysisParamIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."ImageAnalysisParams_ImageAnalysisParamIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."ImageAnalysisParams_ImageAnalysisParamIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."ImageAnalysisParams_ImageAnalysisParamIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."ImageAnalysisParams_ImageAnalysisParamIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."ImageAnalysisParams_ImageAnalysisParamIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."ImageAnalysisParams_ImageAnalysisParamIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3368 (class 0 OID 0)
-- Dependencies: 254
-- Name: TABLE "InstrumentConfig"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."InstrumentConfig" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."InstrumentConfig" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."InstrumentConfig" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."InstrumentConfig" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."InstrumentConfig" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."InstrumentConfig" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."InstrumentConfig" TO "DbBackupUser";


--
-- TOC entry 3370 (class 0 OID 0)
-- Dependencies: 255
-- Name: SEQUENCE "InstrumentConfig_InstrumentIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."InstrumentConfig_InstrumentIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."InstrumentConfig_InstrumentIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."InstrumentConfig_InstrumentIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."InstrumentConfig_InstrumentIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."InstrumentConfig_InstrumentIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."InstrumentConfig_InstrumentIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."InstrumentConfig_InstrumentIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3371 (class 0 OID 0)
-- Dependencies: 256
-- Name: TABLE "QcProcesses"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."QcProcesses" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."QcProcesses" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."QcProcesses" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."QcProcesses" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."QcProcesses" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."QcProcesses" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."QcProcesses" TO "DbBackupUser";


--
-- TOC entry 3373 (class 0 OID 0)
-- Dependencies: 257
-- Name: SEQUENCE "QcProcesses_QcIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."QcProcesses_QcIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."QcProcesses_QcIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."QcProcesses_QcIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."QcProcesses_QcIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."QcProcesses_QcIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."QcProcesses_QcIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."QcProcesses_QcIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3374 (class 0 OID 0)
-- Dependencies: 258
-- Name: TABLE "ReagentInfo"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."ReagentInfo" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."ReagentInfo" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."ReagentInfo" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."ReagentInfo" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."ReagentInfo" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."ReagentInfo" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."ReagentInfo" TO "DbBackupUser";


--
-- TOC entry 3376 (class 0 OID 0)
-- Dependencies: 259
-- Name: SEQUENCE "ReagentInfo_ReagentIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."ReagentInfo_ReagentIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."ReagentInfo_ReagentIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."ReagentInfo_ReagentIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."ReagentInfo_ReagentIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."ReagentInfo_ReagentIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."ReagentInfo_ReagentIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."ReagentInfo_ReagentIdNum_seq" TO "DbBackupUser";



REVOKE ALL ON TABLE "ViCellInstrument"."CellHealthReagents" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."CellHealthReagents" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."CellHealthReagents" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."CellHealthReagents" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."CellHealthReagents" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."CellHealthReagents" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."CellHealthReagents" TO "DbBackupUser";


REVOKE ALL ON SEQUENCE "ViCellInstrument"."CellHealthReagents_IdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."CellHealthReagents_IdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."CellHealthReagents_IdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."CellHealthReagents_IdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."CellHealthReagents_IdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."CellHealthReagents_IdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."CellHealthReagents_IdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3377 (class 0 OID 0)
-- Dependencies: 260
-- Name: TABLE "Roles"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."Roles" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."Roles" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."Roles" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."Roles" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."Roles" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."Roles" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."Roles" TO "DbBackupUser";


--
-- TOC entry 3379 (class 0 OID 0)
-- Dependencies: 261
-- Name: SEQUENCE "Roles_RoleIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."Roles_RoleIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Roles_RoleIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."Roles_RoleIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."Roles_RoleIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Roles_RoleIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Roles_RoleIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."Roles_RoleIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3380 (class 0 OID 0)
-- Dependencies: 262
-- Name: TABLE "SampleItems"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."SampleItems" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."SampleItems" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."SampleItems" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."SampleItems" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."SampleItems" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."SampleItems" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."SampleItems" TO "DbBackupUser";


--
-- TOC entry 3382 (class 0 OID 0)
-- Dependencies: 263
-- Name: SEQUENCE "SampleItems_SampleItemIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."SampleItems_SampleItemIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."SampleItems_SampleItemIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."SampleItems_SampleItemIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."SampleItems_SampleItemIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."SampleItems_SampleItemIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."SampleItems_SampleItemIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."SampleItems_SampleItemIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3383 (class 0 OID 0)
-- Dependencies: 264
-- Name: TABLE "SampleSets"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."SampleSets" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."SampleSets" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."SampleSets" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."SampleSets" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."SampleSets" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."SampleSets" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."SampleSets" TO "DbBackupUser";


--
-- TOC entry 3385 (class 0 OID 0)
-- Dependencies: 265
-- Name: SEQUENCE "SampleSets_SampleSetIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."SampleSets_SampleSetIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."SampleSets_SampleSetIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."SampleSets_SampleSetIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."SampleSets_SampleSetIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."SampleSets_SampleSetIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."SampleSets_SampleSetIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."SampleSets_SampleSetIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3386 (class 0 OID 0)
-- Dependencies: 279
-- Name: TABLE "Scheduler"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."Scheduler" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."Scheduler" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."Scheduler" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."Scheduler" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."Scheduler" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."Scheduler" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."Scheduler" TO "DbBackupUser";


--
-- TOC entry 3385 (class 0 OID 0)
-- Dependencies: 265
-- Name: SEQUENCE "Scheduler_SchedulerConfigIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."Scheduler_SchedulerConfigIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Scheduler_SchedulerConfigIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."Scheduler_SchedulerConfigIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."Scheduler_SchedulerConfigIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Scheduler_SchedulerConfigIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Scheduler_SchedulerConfigIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."Scheduler_SchedulerConfigIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3388 (class 0 OID 0)
-- Dependencies: 266
-- Name: TABLE "SignatureDefinitions"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."SignatureDefinitions" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."SignatureDefinitions" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."SignatureDefinitions" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."SignatureDefinitions" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."SignatureDefinitions" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."SignatureDefinitions" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."SignatureDefinitions" TO "DbBackupUser";


--
-- TOC entry 3390 (class 0 OID 0)
-- Dependencies: 267
-- Name: SEQUENCE "SignatureDefinitions_SignatureDefIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."SignatureDefinitions_SignatureDefIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."SignatureDefinitions_SignatureDefIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."SignatureDefinitions_SignatureDefIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."SignatureDefinitions_SignatureDefIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."SignatureDefinitions_SignatureDefIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."SignatureDefinitions_SignatureDefIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."SignatureDefinitions_SignatureDefIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3391 (class 0 OID 0)
-- Dependencies: 268
-- Name: TABLE "SystemLogs"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."SystemLogs" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."SystemLogs" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."SystemLogs" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."SystemLogs" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."SystemLogs" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."SystemLogs" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."SystemLogs" TO "DbBackupUser";


--
-- TOC entry 3393 (class 0 OID 0)
-- Dependencies: 269
-- Name: SEQUENCE "SystemLogs_EntryIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."SystemLogs_EntryIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."SystemLogs_EntryIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."SystemLogs_EntryIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."SystemLogs_EntryIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."SystemLogs_EntryIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."SystemLogs_EntryIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."SystemLogs_EntryIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3394 (class 0 OID 0)
-- Dependencies: 270
-- Name: TABLE "UserProperties"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."UserProperties" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."UserProperties" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."UserProperties" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."UserProperties" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."UserProperties" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."UserProperties" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."UserProperties" TO "DbBackupUser";


--
-- TOC entry 3396 (class 0 OID 0)
-- Dependencies: 271
-- Name: SEQUENCE "UserProperties_PropertyIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."UserProperties_PropertyIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."UserProperties_PropertyIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."UserProperties_PropertyIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."UserProperties_PropertyIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."UserProperties_PropertyIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."UserProperties_PropertyIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."UserProperties_PropertyIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3397 (class 0 OID 0)
-- Dependencies: 272
-- Name: TABLE "Users"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."Users" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."Users" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."Users" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."Users" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."Users" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."Users" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."Users" TO "DbBackupUser";


--
-- TOC entry 3399 (class 0 OID 0)
-- Dependencies: 273
-- Name: SEQUENCE "Users_UserIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."Users_UserIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Users_UserIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."Users_UserIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."Users_UserIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Users_UserIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Users_UserIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."Users_UserIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3400 (class 0 OID 0)
-- Dependencies: 274
-- Name: TABLE "Workflows"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."Workflows" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."Workflows" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."Workflows" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."Workflows" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."Workflows" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."Workflows" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."Workflows" TO "DbBackupUser";


--
-- TOC entry 3402 (class 0 OID 0)
-- Dependencies: 275
-- Name: SEQUENCE "Workflows_WorkflowIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."Workflows_WorkflowIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Workflows_WorkflowIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."Workflows_WorkflowIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."Workflows_WorkflowIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Workflows_WorkflowIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Workflows_WorkflowIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."Workflows_WorkflowIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3403 (class 0 OID 0)
-- Dependencies: 276
-- Name: TABLE "Worklists"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."Worklists" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."Worklists" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."Worklists" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."Worklists" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."Worklists" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."Worklists" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."Worklists" TO "DbBackupUser";


--
-- TOC entry 3405 (class 0 OID 0)
-- Dependencies: 277
-- Name: SEQUENCE "Worklists_WorklistIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."Worklists_WorklistIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Worklists_WorklistIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."Worklists_WorklistIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."Worklists_WorklistIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Worklists_WorklistIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Worklists_WorklistIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."Worklists_WorklistIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 1959 (class 826 OID 142753)
-- Name: DEFAULT PRIVILEGES FOR TABLES; Type: DEFAULT ACL; Schema: -; Owner: BCIViCellAdmin
--

ALTER DEFAULT PRIVILEGES FOR ROLE "BCIViCellAdmin" REVOKE ALL ON TABLES  FROM "BCIViCellAdmin";
ALTER DEFAULT PRIVILEGES FOR ROLE "BCIViCellAdmin" GRANT ALL ON TABLES  TO "BCIViCellAdmin" WITH GRANT OPTION;
ALTER DEFAULT PRIVILEGES FOR ROLE "BCIViCellAdmin" GRANT ALL ON TABLES  TO "BCIDBAdmin" WITH GRANT OPTION;
ALTER DEFAULT PRIVILEGES FOR ROLE "BCIViCellAdmin" GRANT ALL ON TABLES  TO "ViCellAdmin";
ALTER DEFAULT PRIVILEGES FOR ROLE "BCIViCellAdmin" GRANT ALL ON TABLES  TO "ViCellDBAdmin";
ALTER DEFAULT PRIVILEGES FOR ROLE "BCIViCellAdmin" GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLES  TO "ViCellInstrumentUser";
ALTER DEFAULT PRIVILEGES FOR ROLE "BCIViCellAdmin" GRANT SELECT,REFERENCES,TRIGGER ON TABLES  TO "DbBackupUser";


-- Completed on 2020-12-29 11:08:04

--
-- PostgreSQL database dump complete
--



--
-- PostgreSQL database dump
--

-- Dumped from database version 10.12
-- Dumped by pg_dump version 10.12

-- Started on 2020-12-29 11:08:04

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET check_function_bodies = false;
SET xmloption = content;
SET client_min_messages = warning;
SET row_security = off;

DROP DATABASE "ViCellDB";
--
-- TOC entry 3305 (class 1262 OID 142311)
-- Name: ViCellDB_template; Type: DATABASE; Schema: -; Owner: BCIViCellAdmin
--

CREATE DATABASE "ViCellDB" WITH TEMPLATE = template0 ENCODING = 'UTF8' LC_COLLATE = 'English_United States.1252' LC_CTYPE = 'English_United States.1252';

ALTER DATABASE "ViCellDB" OWNER TO "BCIViCellAdmin";

\connect "ViCellDB"

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET check_function_bodies = false;
SET xmloption = content;
SET client_min_messages = warning;
SET row_security = off;

--
-- TOC entry 9 (class 2615 OID 142312)
-- Name: ViCellData; Type: SCHEMA; Schema: -; Owner: BCIViCellAdmin
--

CREATE SCHEMA "ViCellData";


ALTER SCHEMA "ViCellData" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 4 (class 2615 OID 142313)
-- Name: ViCellInstrument; Type: SCHEMA; Schema: -; Owner: BCIViCellAdmin
--

CREATE SCHEMA "ViCellInstrument";


ALTER SCHEMA "ViCellInstrument" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 1 (class 3079 OID 12924)
-- Name: plpgsql; Type: EXTENSION; Schema: -; Owner: 
--

CREATE EXTENSION IF NOT EXISTS plpgsql WITH SCHEMA pg_catalog;


--
-- TOC entry 3310 (class 0 OID 0)
-- Dependencies: 1
-- Name: EXTENSION plpgsql; Type: COMMENT; Schema: -; Owner: 
--

COMMENT ON EXTENSION plpgsql IS 'PL/pgSQL procedural language';


--
-- TOC entry 588 (class 1247 OID 142316)
-- Name: ad_settings; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.ad_settings AS (
	servername character varying,
	server_addr character varying,
	port_number integer,
	base_dn character varying,
	enabled boolean
);


ALTER TYPE public.ad_settings OWNER TO "BCIViCellAdmin";

--
-- TOC entry 591 (class 1247 OID 142319)
-- Name: af_settings; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.af_settings AS (
	save_image boolean,
	coarse_start integer,
	coarse_end integer,
	coarse_step smallint,
	fine_range integer,
	fine_step smallint,
	sharpness_low_threshold integer
);


ALTER TYPE public.af_settings OWNER TO "BCIViCellAdmin";

--
-- TOC entry 673 (class 1247 OID 142322)
-- Name: analysis_input_params; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.analysis_input_params AS (
	key smallint,
	skey smallint,
	sskey smallint,
	value real,
	polarity smallint
);


ALTER TYPE public.analysis_input_params OWNER TO "BCIViCellAdmin";

--
-- TOC entry 676 (class 1247 OID 142325)
-- Name: blob_characteristics; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.blob_characteristics AS (
	key smallint,
	skey smallint,
	sskey smallint,
	value real
);


ALTER TYPE public.blob_characteristics OWNER TO "BCIViCellAdmin";

--
-- TOC entry 679 (class 1247 OID 142328)
-- Name: blob_point; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.blob_point AS (
	startx smallint,
	starty smallint
);


ALTER TYPE public.blob_point OWNER TO "BCIViCellAdmin";

--
-- TOC entry 682 (class 1247 OID 142331)
-- Name: blob_data; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.blob_data AS (
	blob_info public.blob_characteristics[],
	blob_center public.blob_point,
	blob_outline public.blob_point[]
);


ALTER TYPE public.blob_data OWNER TO "BCIViCellAdmin";

--
-- TOC entry 685 (class 1247 OID 142334)
-- Name: blob_info_array; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.blob_info_array AS (
	blob_index integer,
	blob_info_list public.blob_characteristics[]
);


ALTER TYPE public.blob_info_array OWNER TO "BCIViCellAdmin";

--
-- TOC entry 688 (class 1247 OID 142337)
-- Name: blob_outline_array; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.blob_outline_array AS (
	blob_index integer,
	blob_outline public.blob_point[]
);


ALTER TYPE public.blob_outline_array OWNER TO "BCIViCellAdmin";

--
-- TOC entry 691 (class 1247 OID 142340)
-- Name: blob_rect; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.blob_rect AS (
	start_x smallint,
	start_y smallint,
	width smallint,
	height smallint
);


ALTER TYPE public.blob_rect OWNER TO "BCIViCellAdmin";

--
-- TOC entry 694 (class 1247 OID 142343)
-- Name: cal_consumable; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.cal_consumable AS (
	label character varying,
	lot_id character varying,
	cal_type smallint,
	expiration_date timestamp without time zone,
	assay_value real
);


ALTER TYPE public.cal_consumable OWNER TO "BCIViCellAdmin";

--
-- TOC entry 697 (class 1247 OID 142346)
-- Name: cluster_data; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.cluster_data AS (
	cell_count smallint,
	cluster_polygon public.blob_point[],
	cluster_box_startx smallint,
	cluster_box_starty smallint,
	cluster_box_width smallint,
	cluster_box_height smallint
);


ALTER TYPE public.cluster_data OWNER TO "BCIViCellAdmin";

--
-- TOC entry 700 (class 1247 OID 142349)
-- Name: column_display_info; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.column_display_info AS (
	col_type smallint,
	col_position smallint,
	col_width smallint,
	visible boolean
);


ALTER TYPE public.column_display_info OWNER TO "BCIViCellAdmin";

--
-- TOC entry 703 (class 1247 OID 142352)
-- Name: email_settings; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.email_settings AS (
	server_addr character varying,
	port_number integer,
	authenticate boolean,
	username character varying,
	password character varying
);


ALTER TYPE public.email_settings OWNER TO "BCIViCellAdmin";

--
-- TOC entry 706 (class 1247 OID 142355)
-- Name: illuminator_info; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.illuminator_info AS (
	type smallint,
	index smallint
);


ALTER TYPE public.illuminator_info OWNER TO "BCIViCellAdmin";

--
-- TOC entry 709 (class 1247 OID 142358)
-- Name: input_config_params; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.input_config_params AS (
	config_params_enum smallint,
	config_value double precision
);


ALTER TYPE public.input_config_params OWNER TO "BCIViCellAdmin";

--
-- TOC entry 712 (class 1247 OID 142361)
-- Name: int16_map_pair; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.int16_map_pair AS (
	channel smallint,
	max_num_peaks smallint
);


ALTER TYPE public.int16_map_pair OWNER TO "BCIViCellAdmin";

--
-- TOC entry 715 (class 1247 OID 142364)
-- Name: language_info; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.language_info AS (
	language_id integer,
	language_name character varying,
	locale_tag character varying,
	active boolean
);


ALTER TYPE public.language_info OWNER TO "BCIViCellAdmin";

--
-- TOC entry 718 (class 1247 OID 142367)
-- Name: rfid_sim_info; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.rfid_sim_info AS (
	set_valid_tag_data boolean,
	total_tags smallint,
	main_bay_file character varying,
	door_left_file character varying,
	door_right_file character varying
);


ALTER TYPE public.rfid_sim_info OWNER TO "BCIViCellAdmin";

--
-- TOC entry 721 (class 1247 OID 142370)
-- Name: run_options_info; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.run_options_info AS (
	sample_set_name character varying,
	sample_name character varying,
	save_image_count smallint,
	save_nth_image smallint,
	results_export boolean,
	results_export_folder character varying,
	append_results_export boolean,
	append_results_export_folder character varying,
	result_filename character varying,
	result_folder character varying,
	auto_export_pdf boolean,
	csv_folder character varying,
	wash_type smallint,
	dilution smallint,
	bpqc_cell_type_index smallint
);


ALTER TYPE public.run_options_info OWNER TO "BCIViCellAdmin";

--
-- TOC entry 724 (class 1247 OID 142373)
-- Name: signature_info; Type: TYPE; Schema: public; Owner: BCIViCellAdmin
--

CREATE TYPE public.signature_info AS (
	username character varying,
	short_tag character varying,
	long_tag character varying,
	signature_time timestamp without time zone,
	signature_hash character varying
);


ALTER TYPE public.signature_info OWNER TO "BCIViCellAdmin";

SET default_tablespace = '';

SET default_with_oids = false;

--
-- TOC entry 218 (class 1259 OID 142374)
-- Name: Analyses; Type: TABLE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellData"."Analyses" (
    "AnalysisIdNum" bigint NOT NULL,
    "AnalysisID" uuid NOT NULL,
    "SampleID" uuid,
    "ImageSetID" uuid,
    "SummaryResultID" uuid,
    "SResultID" uuid,
    "RunUserID" uuid,
    "AnalysisDate" timestamp without time zone,
    "InstrumentSN" character varying,
    "BioProcessID" uuid,
    "QcProcessID" uuid,
    "WorkflowID" uuid,
    "ImageSequenceCount" smallint,
    "ImageSequenceIDList" uuid[],
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellData"."Analyses" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 219 (class 1259 OID 142381)
-- Name: Analyses_AnalysisIdNum_seq; Type: SEQUENCE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellData"."Analyses_AnalysisIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellData"."Analyses_AnalysisIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3312 (class 0 OID 0)
-- Dependencies: 219
-- Name: Analyses_AnalysisIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellData"."Analyses_AnalysisIdNum_seq" OWNED BY "ViCellData"."Analyses"."AnalysisIdNum";


--
-- TOC entry 220 (class 1259 OID 142383)
-- Name: DetailedResults; Type: TABLE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellData"."DetailedResults" (
    "DetailedResultIdNum" bigint NOT NULL,
    "DetailedResultID" uuid NOT NULL,
    "SampleID" uuid NOT NULL,
    "ImageID" uuid NOT NULL,
    "AnalysisID" uuid NOT NULL,
    "OwnerID" uuid NOT NULL,
    "ResultDate" timestamp without time zone NOT NULL,
    "ProcessingStatus" smallint NOT NULL,
    "TotCumulativeImages" smallint,
    "TotalCellsGP" integer,
    "TotalCellsPOI" integer,
    "POIPopulationPercent" double precision,
    "CellConcGP" double precision,
    "CellConcPOI" double precision,
    "AvgDiamGP" double precision,
    "AvgDiamPOI" double precision,
    "AvgCircularityGP" double precision,
    "AvgCircularityPOI" double precision,
    "AvgSharpnessGP" double precision,
    "AvgSharpnessPOI" double precision,
    "AvgEccentricityGP" double precision,
    "AvgEccentricityPOI" double precision,
    "AvgAspectRatioGP" double precision,
    "AvgAspectRatioPOI" double precision,
    "AvgRoundnessGP" double precision,
    "AvgRoundnessPOI" double precision,
    "AvgRawCellSpotBrightnessGP" double precision,
    "AvgRawCellSpotBrightnessPOI" double precision,
    "AvgCellSpotBrightnessGP" double precision,
    "AvgCellSpotBrightnessPOI" double precision,
    "AvgBkgndIntensity" double precision,
    "TotalBubbleCount" integer,
    "LargeClusterCount" integer,
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellData"."DetailedResults" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 221 (class 1259 OID 142387)
-- Name: DetailedResults_DetailedResultIdNum_seq; Type: SEQUENCE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellData"."DetailedResults_DetailedResultIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellData"."DetailedResults_DetailedResultIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3315 (class 0 OID 0)
-- Dependencies: 221
-- Name: DetailedResults_DetailedResultIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellData"."DetailedResults_DetailedResultIdNum_seq" OWNED BY "ViCellData"."DetailedResults"."DetailedResultIdNum";


--
-- TOC entry 222 (class 1259 OID 142389)
-- Name: ImageReferences; Type: TABLE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellData"."ImageReferences" (
    "ImageIdNum" bigint NOT NULL,
    "ImageID" uuid NOT NULL,
    "ImageSequenceID" uuid NOT NULL,
    "ImageChannel" smallint NOT NULL,
    "ImageFileName" character varying,
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellData"."ImageReferences" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3317 (class 0 OID 0)
-- Dependencies: 222
-- Name: TABLE "ImageReferences"; Type: COMMENT; Schema: ViCellData; Owner: BCIViCellAdmin
--

COMMENT ON TABLE "ViCellData"."ImageReferences" IS 'References to images and image storage locations; raw images are not stored in the database;';


--
-- TOC entry 223 (class 1259 OID 142396)
-- Name: ImageReferences_ImageIdNum_seq; Type: SEQUENCE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellData"."ImageReferences_ImageIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellData"."ImageReferences_ImageIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3319 (class 0 OID 0)
-- Dependencies: 223
-- Name: ImageReferences_ImageIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellData"."ImageReferences_ImageIdNum_seq" OWNED BY "ViCellData"."ImageReferences"."ImageIdNum";


--
-- TOC entry 224 (class 1259 OID 142398)
-- Name: ImageResults; Type: TABLE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellData"."ImageResults" (
    "ResultIdNum" bigint NOT NULL,
    "ResultID" uuid NOT NULL,
    "SampleID" uuid NOT NULL,
    "ImageID" uuid NOT NULL,
    "AnalysisID" uuid NOT NULL,
    "ImageSeqNum" smallint NOT NULL,
    "DetailedResultID" uuid NOT NULL,
    "MaxNumOfPeaksFlChanMap" public.int16_map_pair[],
    "NumBlobs" smallint,
    "BlobInfoListStr" character varying,
    "BlobCenterListStr" character varying,
    "BlobOutlineListStr" character varying,
    "NumClusters" smallint,
    "ClusterCellCountList" smallint[],
    "ClusterPolygonListStr" character varying,
    "ClusterRectListStr" character varying,
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellData"."ImageResults" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 225 (class 1259 OID 142405)
-- Name: ImageResults_ResultIdNum_seq; Type: SEQUENCE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellData"."ImageResults_ResultIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellData"."ImageResults_ResultIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3322 (class 0 OID 0)
-- Dependencies: 225
-- Name: ImageResults_ResultIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellData"."ImageResults_ResultIdNum_seq" OWNED BY "ViCellData"."ImageResults"."ResultIdNum";


--
-- TOC entry 226 (class 1259 OID 142407)
-- Name: ImageSequences; Type: TABLE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellData"."ImageSequences" (
    "ImageSequenceIdNum" bigint NOT NULL,
    "ImageSequenceID" uuid NOT NULL,
    "ImageSetID" uuid NOT NULL,
    "SequenceNum" smallint NOT NULL,
    "ImageCount" smallint NOT NULL,
    "FlChannels" smallint NOT NULL,
    "ImageIDList" uuid[],
    "ImageSequenceFolder" character varying NOT NULL,
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellData"."ImageSequences" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3324 (class 0 OID 0)
-- Dependencies: 226
-- Name: TABLE "ImageSequences"; Type: COMMENT; Schema: ViCellData; Owner: BCIViCellAdmin
--

COMMENT ON TABLE "ViCellData"."ImageSequences" IS 'records of each image sequence (image composite) taken for a particular sequence number (typically, 1 - 100) for a sample; the record details if multiple images are taken at a given point in the image sequence (as would occur for fluorescence);';


--
-- TOC entry 227 (class 1259 OID 142414)
-- Name: ImageSequences_ImageSequenceIdNum_seq; Type: SEQUENCE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellData"."ImageSequences_ImageSequenceIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellData"."ImageSequences_ImageSequenceIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3326 (class 0 OID 0)
-- Dependencies: 227
-- Name: ImageSequences_ImageSequenceIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellData"."ImageSequences_ImageSequenceIdNum_seq" OWNED BY "ViCellData"."ImageSequences"."ImageSequenceIdNum";


--
-- TOC entry 228 (class 1259 OID 142416)
-- Name: ImageSets; Type: TABLE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellData"."ImageSets" (
    "ImageSetIdNum" bigint NOT NULL,
    "ImageSetID" uuid NOT NULL,
    "SampleID" uuid NOT NULL,
    "CreationDate" timestamp without time zone NOT NULL,
    "ImageSetFolder" character varying NOT NULL,
    "ImageSequenceCount" smallint NOT NULL,
    "ImageSequenceIDList" uuid[],
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellData"."ImageSets" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3328 (class 0 OID 0)
-- Dependencies: 228
-- Name: TABLE "ImageSets"; Type: COMMENT; Schema: ViCellData; Owner: BCIViCellAdmin
--

COMMENT ON TABLE "ViCellData"."ImageSets" IS 'Record documenting all images belonging to a sample';


--
-- TOC entry 229 (class 1259 OID 142423)
-- Name: ImageSets_ImageSetIdNum_seq; Type: SEQUENCE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellData"."ImageSets_ImageSetIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellData"."ImageSets_ImageSetIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3330 (class 0 OID 0)
-- Dependencies: 229
-- Name: ImageSets_ImageSetIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellData"."ImageSets_ImageSetIdNum_seq" OWNED BY "ViCellData"."ImageSets"."ImageSetIdNum";


--
-- TOC entry 230 (class 1259 OID 142425)
-- Name: SResults; Type: TABLE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellData"."SResults" (
    "SResultIdNum" bigint NOT NULL,
    "SResultID" uuid NOT NULL,
    "CumulativeDetailedResultID" uuid NOT NULL,
    "ImageResultIDList" uuid[] NOT NULL,
    "AnalysisID" uuid NOT NULL,
    "SampleID" uuid NOT NULL,
    "ProcessingSettingsID" uuid NOT NULL,
    "CumMaxNumOfPeaksFlChan" public.int16_map_pair[],
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellData"."SResults" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 231 (class 1259 OID 142432)
-- Name: SResults_SResultIdNum_seq; Type: SEQUENCE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellData"."SResults_SResultIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellData"."SResults_SResultIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3333 (class 0 OID 0)
-- Dependencies: 231
-- Name: SResults_SResultIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellData"."SResults_SResultIdNum_seq" OWNED BY "ViCellData"."SResults"."SResultIdNum";


--
-- TOC entry 232 (class 1259 OID 142434)
-- Name: SampleProperties; Type: TABLE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellData"."SampleProperties" (
    "SampleIdNum" bigint NOT NULL,
    "SampleID" uuid NOT NULL,
    "SampleStatus" smallint,
    "SampleName" character varying,
    "CellTypeID" uuid NOT NULL,
    "CellTypeIndex" integer,
    "AnalysisDefinitionID" uuid,
    "AnalysisDefinitionIndex" integer,
    "Label" character varying,
    "BioProcessID" uuid,
    "QcProcessID" uuid,
    "WorkflowID" uuid,
    "Comments" character varying,
    "WashType" smallint,
    "Dilution" smallint,
    "OwnerUserID" uuid,
    "RunUserID" uuid,
    "AcquisitionDate" timestamp without time zone,
    "ImageSetID" uuid,
    "DustRefImageSetID" uuid,
    "InstrumentSN" character varying,
    "ImageAnalysisParamID" uuid NOT NULL,
    "NumReagents" smallint,
    "ReagentTypeNameList" character varying[],
    "ReagentPackNumList" character varying[],
    "PackLotNumList" character varying[],
    "PackLotExpirationList" bigint[],
    "PackInServiceList" bigint[],
    "PackServiceExpirationList" bigint[],
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellData"."SampleProperties" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 233 (class 1259 OID 142441)
-- Name: SampleProperties_SampleIdNum_seq; Type: SEQUENCE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellData"."SampleProperties_SampleIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellData"."SampleProperties_SampleIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3336 (class 0 OID 0)
-- Dependencies: 233
-- Name: SampleProperties_SampleIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellData"."SampleProperties_SampleIdNum_seq" OWNED BY "ViCellData"."SampleProperties"."SampleIdNum";


--
-- TOC entry 234 (class 1259 OID 142443)
-- Name: SummaryResults; Type: TABLE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellData"."SummaryResults" (
    "SummaryResultIdNum" bigint NOT NULL,
    "SummaryResultID" uuid NOT NULL,
    "SampleID" uuid NOT NULL,
    "ImageSetID" uuid NOT NULL,
    "AnalysisID" uuid NOT NULL,
    "ResultDate" timestamp without time zone NOT NULL,
    "SignatureList" public.signature_info[],
    "ImageAnalysisParamID" uuid NOT NULL,
    "AnalysisDefID" uuid,
    "AnalysisParamID" uuid NOT NULL,
    "CellTypeID" uuid NOT NULL,
    "CellTypeIndex" integer,
    "ProcessingStatus" smallint NOT NULL,
    "TotCumulativeImages" smallint,
    "TotalCellsGP" integer,
    "TotalCellsPOI" integer,
    "POIPopulationPercent" real,
    "CellConcGP" real,
    "CellConcPOI" real,
    "AvgDiamGP" real,
    "AvgDiamPOI" real,
    "AvgCircularityGP" real,
    "AvgCircularityPOI" real,
    "CoefficientOfVariance" real,
    "AvgCellsPerImage" smallint,
    "AvgBkgndIntensity" smallint,
    "TotalBubbleCount" smallint,
    "LargeClusterCount" smallint,
    "QcStatus" smallint,
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellData"."SummaryResults" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 235 (class 1259 OID 142450)
-- Name: SummaryResults_SummaryResultIdNum_seq; Type: SEQUENCE; Schema: ViCellData; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellData"."SummaryResults_SummaryResultIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellData"."SummaryResults_SummaryResultIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3339 (class 0 OID 0)
-- Dependencies: 235
-- Name: SummaryResults_SummaryResultIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellData"."SummaryResults_SummaryResultIdNum_seq" OWNED BY "ViCellData"."SummaryResults"."SummaryResultIdNum";


--
-- TOC entry 236 (class 1259 OID 142452)
-- Name: AnalysisDefinitions; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."AnalysisDefinitions" (
    "AnalysisDefinitionIdNum" bigint NOT NULL,
    "AnalysisDefinitionID" uuid NOT NULL,
    "AnalysisDefinitionIndex" integer,
    "AnalysisDefinitionName" character varying,
    "NumReagents" smallint,
    "ReagentTypeIndexList" integer[],
    "MixingCycles" smallint,
    "NumIlluminators" smallint,
    "IlluminatorsIndexList" smallint[],
    "NumAnalysisParams" smallint,
    "AnalysisParamIDList" uuid[],
    "PopulationParamExists" boolean DEFAULT false NOT NULL,
    "PopulationParamID" uuid,
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellInstrument"."AnalysisDefinitions" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 237 (class 1259 OID 142460)
-- Name: AnalysisDefinitions_AnalysisDefinitionIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."AnalysisDefinitions_AnalysisDefinitionIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."AnalysisDefinitions_AnalysisDefinitionIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3342 (class 0 OID 0)
-- Dependencies: 237
-- Name: AnalysisDefinitions_AnalysisDefinitionIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."AnalysisDefinitions_AnalysisDefinitionIdNum_seq" OWNED BY "ViCellInstrument"."AnalysisDefinitions"."AnalysisDefinitionIdNum";


--
-- TOC entry 238 (class 1259 OID 142462)
-- Name: AnalysisInputSettings; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."AnalysisInputSettings" (
    "SettingsIdNum" bigint NOT NULL,
    "SettingsID" uuid NOT NULL,
    "InputConfigParamMap" public.input_config_params[],
    "CellIdentParamList" public.analysis_input_params[],
    "POIIdentParamList" public.analysis_input_params[]
);


ALTER TABLE "ViCellInstrument"."AnalysisInputSettings" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 239 (class 1259 OID 142468)
-- Name: AnalysisInputSettings_SettingsIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."AnalysisInputSettings_SettingsIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."AnalysisInputSettings_SettingsIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3345 (class 0 OID 0)
-- Dependencies: 239
-- Name: AnalysisInputSettings_SettingsIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."AnalysisInputSettings_SettingsIdNum_seq" OWNED BY "ViCellInstrument"."AnalysisInputSettings"."SettingsIdNum";


--
-- TOC entry 240 (class 1259 OID 142470)
-- Name: AnalysisParams; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."AnalysisParams" (
    "AnalysisParamIdNum" bigint NOT NULL,
    "AnalysisParamID" uuid NOT NULL,
    "IsInitialized" boolean,
    "AnalysisParamLabel" character varying,
    "CharacteristicKey" smallint,
    "CharacteristicSKey" smallint,
    "CharacteristicSSKey" smallint,
    "ThresholdValue" real,
    "AboveThreshold" boolean,
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellInstrument"."AnalysisParams" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 241 (class 1259 OID 142477)
-- Name: AnalysisParams_AnalysisParamIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."AnalysisParams_AnalysisParamIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."AnalysisParams_AnalysisParamIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3348 (class 0 OID 0)
-- Dependencies: 241
-- Name: AnalysisParams_AnalysisParamIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."AnalysisParams_AnalysisParamIdNum_seq" OWNED BY "ViCellInstrument"."AnalysisParams"."AnalysisParamIdNum";


--
-- TOC entry 242 (class 1259 OID 142479)
-- Name: BioProcesses; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."BioProcesses" (
    "BioProcessIdNum" bigint NOT NULL,
    "BioProcessID" uuid NOT NULL,
    "BioProcessName" character varying NOT NULL,
    "BioProcessSequence" character varying,
    "ReactorName" character varying,
    "CelltypeID" uuid,
    "CellTypeIndex" integer,
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellInstrument"."BioProcesses" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 243 (class 1259 OID 142486)
-- Name: BioProcesses_BioProcessIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."BioProcesses_BioProcessIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."BioProcesses_BioProcessIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3351 (class 0 OID 0)
-- Dependencies: 243
-- Name: BioProcesses_BioProcessIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."BioProcesses_BioProcessIdNum_seq" OWNED BY "ViCellInstrument"."BioProcesses"."BioProcessIdNum";


--
-- TOC entry 244 (class 1259 OID 142488)
-- Name: Calibrations; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."Calibrations" (
    "CalibrationIdNum" bigint NOT NULL,
    "CalibrationID" uuid NOT NULL,
    "InstrumentSN" character varying,
    "CalibrationDate" timestamp without time zone,
    "CalibrationUserID" uuid,
    "CalibrationType" smallint,
    "Slope" double precision,
    "Intercept" double precision,
    "ImageCount" smallint,
    "CalQueueID" uuid,
    "ConsumablesList" public.cal_consumable[],
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellInstrument"."Calibrations" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 245 (class 1259 OID 142495)
-- Name: Calibrations_CalibrationIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."Calibrations_CalibrationIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."Calibrations_CalibrationIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3354 (class 0 OID 0)
-- Dependencies: 245
-- Name: Calibrations_CalibrationIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."Calibrations_CalibrationIdNum_seq" OWNED BY "ViCellInstrument"."Calibrations"."CalibrationIdNum";


--
-- TOC entry 246 (class 1259 OID 142497)
-- Name: CellTypes; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."CellTypes" (
    "CellTypeIdNum" bigint NOT NULL,
    "CellTypeID" uuid NOT NULL,
    "CellTypeIndex" integer,
    "CellTypeName" character varying,
    "Retired" boolean DEFAULT false,
    "MaxImages" smallint,
    "AspirationCycles" smallint,
    "MinDiamMicrons" real,
    "MaxDiamMicrons" real,
    "MinCircularity" real,
    "SharpnessLimit" real,
    "NumCellIdentParams" smallint,
    "CellIdentParamIDList" uuid[],
    "DeclusterSetting" smallint,
    "RoiExtent" real,
    "RoiXPixels" integer,
    "RoiYPixels" integer,
    "NumAnalysisSpecializations" smallint,
    "AnalysisSpecializationIDList" uuid[],
    "CalculationAdjustmentFactor" real DEFAULT 0.0 NOT NULL,
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellInstrument"."CellTypes" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 247 (class 1259 OID 142506)
-- Name: CellTypes_CellTypeIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."CellTypes_CellTypeIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."CellTypes_CellTypeIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3357 (class 0 OID 0)
-- Dependencies: 247
-- Name: CellTypes_CellTypeIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."CellTypes_CellTypeIdNum_seq" OWNED BY "ViCellInstrument"."CellTypes"."CellTypeIdNum";


--
-- TOC entry 248 (class 1259 OID 142508)
-- Name: IlluminatorTypes; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."IlluminatorTypes" (
    "IlluminatorIdNum" bigint NOT NULL,
    "IlluminatorIndex" smallint NOT NULL,
    "IlluminatorType" smallint NOT NULL,
    "IlluminatorName" character varying NOT NULL,
    "PositionNum" smallint,
    "Tolerance" real DEFAULT 0.1,
    "MaxVoltage" integer DEFAULT 0,
    "IlluminatorWavelength" smallint,
    "EmissionWavelength" smallint,
    "ExposureTimeMs" smallint,
    "PercentPower" smallint,
    "SimmerVoltage" integer,
    "Ltcd" smallint DEFAULT 100,
    "Ctld" smallint DEFAULT 100,
    "FeedbackPhotoDiode" smallint DEFAULT 1,
    "Protected" boolean DEFAULT true NOT NULL
);


ALTER TABLE "ViCellInstrument"."IlluminatorTypes" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 249 (class 1259 OID 142520)
-- Name: IlluminatorTypes_IlluminatorIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."IlluminatorTypes_IlluminatorIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."IlluminatorTypes_IlluminatorIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3360 (class 0 OID 0)
-- Dependencies: 249
-- Name: IlluminatorTypes_IlluminatorIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."IlluminatorTypes_IlluminatorIdNum_seq" OWNED BY "ViCellInstrument"."IlluminatorTypes"."IlluminatorIdNum";


--
-- TOC entry 250 (class 1259 OID 142522)
-- Name: ImageAnalysisCellIdentParams; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."ImageAnalysisCellIdentParams" (
    "IdentParamIdNum" bigint NOT NULL,
    "IdentParamID" uuid NOT NULL,
    "CharacteristicKey" smallint,
    "CharacteristicSKey" smallint,
    "CharacteristicSSKey" smallint,
    "ParamValue" real,
    "ValueTest" smallint,
    "Protected" boolean DEFAULT true NOT NULL
);


ALTER TABLE "ViCellInstrument"."ImageAnalysisCellIdentParams" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 251 (class 1259 OID 142526)
-- Name: ImageAnalysisCellIdentParams_IdentParamIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."ImageAnalysisCellIdentParams_IdentParamIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."ImageAnalysisCellIdentParams_IdentParamIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3363 (class 0 OID 0)
-- Dependencies: 251
-- Name: ImageAnalysisCellIdentParams_IdentParamIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."ImageAnalysisCellIdentParams_IdentParamIdNum_seq" OWNED BY "ViCellInstrument"."ImageAnalysisCellIdentParams"."IdentParamIdNum";


--
-- TOC entry 252 (class 1259 OID 142528)
-- Name: ImageAnalysisParams; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."ImageAnalysisParams" (
    "ImageAnalysisParamIdNum" bigint NOT NULL,
    "ImageAnalysisParamID" uuid NOT NULL,
    "AlgorithmMode" integer,
    "BubbleMode" boolean,
    "DeclusterMode" boolean,
    "SubPeakAnalysisMode" boolean,
    "DilutionFactor" integer,
    "ROIXcoords" integer,
    "ROIYcoords" integer,
    "DeclusterAccumulatorThreshLow" integer,
    "DeclusterMinDistanceThreshLow" integer,
    "DeclusterAccumulatorThreshMed" integer,
    "DeclusterMinDistanceThreshMed" integer,
    "DeclusterAccumulatorThreshHigh" integer,
    "DeclusterMinDistanceThreshHigh" integer,
    "FovDepthMM" double precision,
    "PixelFovMM" double precision,
    "SizingSlope" double precision,
    "SizingIntercept" double precision,
    "ConcSlope" double precision,
    "ConcIntercept" double precision,
    "ConcImageControlCnt" integer,
    "BubbleMinSpotAreaPrcnt" real,
    "BubbleMinSpotAreaBrightness" real,
    "BubbleRejectImgAreaPrcnt" real,
    "VisibleCellSpotArea" double precision,
    "FlScalableROI" double precision,
    "FLPeakPercent" double precision,
    "NominalBkgdLevel" double precision,
    "BkgdIntensityTolerance" double precision,
    "CenterSpotMinIntensityLimit" double precision,
    "PeakIntensitySelectionAreaLimit" double precision,
    "CellSpotBrightnessExclusionThreshold" double precision,
    "HotPixelEliminationMode" double precision,
    "ImgBotAndRightBoundaryAnnotationMode" double precision,
    "SmallParticleSizingCorrection" double precision,
    "Protected" boolean DEFAULT true NOT NULL
);


ALTER TABLE "ViCellInstrument"."ImageAnalysisParams" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 253 (class 1259 OID 142532)
-- Name: ImageAnalysisParams_ImageAnalysisParamIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."ImageAnalysisParams_ImageAnalysisParamIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."ImageAnalysisParams_ImageAnalysisParamIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3366 (class 0 OID 0)
-- Dependencies: 253
-- Name: ImageAnalysisParams_ImageAnalysisParamIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."ImageAnalysisParams_ImageAnalysisParamIdNum_seq" OWNED BY "ViCellInstrument"."ImageAnalysisParams"."ImageAnalysisParamIdNum";


--
-- TOC entry 254 (class 1259 OID 142534)
-- Name: InstrumentConfig; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."InstrumentConfig" (
    "InstrumentIdNum" bigint NOT NULL,
    "InstrumentSN" character varying DEFAULT 'ViCellInstrumentSN'::character varying NOT NULL,
    "InstrumentType" smallint DEFAULT 1 NOT NULL,
    "DeviceName" character varying,
    "UIVersion" character varying,
    "SoftwareVersion" character varying,
    "AnalysisSWVersion" character varying,
    "FirmwareVersion" character varying,
    "BrightFieldLedType" smallint,
    "CameraType" smallint,
    "CameraFWVersion" character varying,
    "CameraConfig" character varying,
    "PumpType" smallint,
    "PumpFWVersion" character varying,
    "PumpConfig" character varying,
    "IlluminatorsInfoList" public.illuminator_info[],
    "IlluminatorConfig" character varying,
    "ConfigType" smallint,
    "LogName" character varying,
    "LogMaxSize" integer,
    "LogSensitivity" character varying,
    "MaxLogs" smallint,
    "AlwaysFlush" boolean,
    "CameraErrorLogName" character varying,
    "CameraErrorLogMaxSize" integer,
    "StorageErrorLogName" character varying,
    "StorageErrorLogMaxSize" integer,
    "CarouselThetaHomeOffset" integer DEFAULT 0,
    "CarouselRadiusOffset" integer DEFAULT 0,
    "PlateThetaHomePosOffset" integer DEFAULT 0,
    "PlateThetaCalPos" integer DEFAULT 0,
    "PlateRadiusCenterPos" integer DEFAULT 0,
    "SaveImage" smallint,
    "FocusPosition" integer,
    "AutoFocus" public.af_settings,
    "AbiMaxImageCount" smallint,
    "SampleNudgeVolume" smallint,
    "SampleNudgeSpeed" smallint,
    "FlowCellDepth" real,
    "FlowCellDepthConstant" real,
    "RfidSim" public.rfid_sim_info,
    "LegacyData" boolean,
    "CarouselSimulator" boolean,
    "NightlyCleanOffset" smallint,
    "LastNightlyClean" timestamp without time zone,
    "SecurityMode" smallint,
    "InactivityTimeout" smallint,
    "PasswordExpiration" smallint,
    "NormalShutdown" boolean,
    "NextAnalysisDefIndex" integer DEFAULT 0,
    "NextFactoryCellTypeIndex" integer DEFAULT 0,
    "NextUserCellTypeIndex" bigint DEFAULT '2147483648'::bigint,
    "SamplesProcessed" integer DEFAULT 0,
    "DiscardCapacity" smallint DEFAULT 120,
    "EmailServer" public.email_settings,
    "ADSettings" public.ad_settings,
    "LanguageList" public.language_info[],
    "DefaultDisplayColumns" public.column_display_info[],
    "RunOptionDefaults" public.run_options_info,
    "AutomationInstalled" boolean DEFAULT false,
    "AutomationEnabled" boolean DEFAULT false,
    "ACupEnabled" boolean DEFAULT false,
    "AutomationPort" integer DEFAULT 0,
    "Protected" boolean DEFAULT true NOT NULL
);


ALTER TABLE "ViCellInstrument"."InstrumentConfig" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 255 (class 1259 OID 142556)
-- Name: InstrumentConfig_InstrumentIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."InstrumentConfig_InstrumentIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."InstrumentConfig_InstrumentIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3369 (class 0 OID 0)
-- Dependencies: 255
-- Name: InstrumentConfig_InstrumentIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."InstrumentConfig_InstrumentIdNum_seq" OWNED BY "ViCellInstrument"."InstrumentConfig"."InstrumentIdNum";


--
-- TOC entry 256 (class 1259 OID 142558)
-- Name: QcProcesses; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."QcProcesses" (
    "QcIdNum" bigint NOT NULL,
    "QcID" uuid NOT NULL,
    "QcName" character varying NOT NULL,
    "QcType" smallint,
    "CellTypeID" uuid,
    "CellTypeIndex" integer,
    "LotInfo" character varying,
    "LotExpiration" timestamp without time zone,
    "AssayValue" double precision,
    "AllowablePercentage" double precision,
    "QcSequence" character varying,
    "Comments" character varying,
    "Retired" boolean DEFAULT false,
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellInstrument"."QcProcesses" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 257 (class 1259 OID 142565)
-- Name: QcProcesses_QcIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."QcProcesses_QcIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."QcProcesses_QcIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3372 (class 0 OID 0)
-- Dependencies: 257
-- Name: QcProcesses_QcIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."QcProcesses_QcIdNum_seq" OWNED BY "ViCellInstrument"."QcProcesses"."QcIdNum";


--
-- TOC entry 258 (class 1259 OID 142567)
-- Name: ReagentInfo; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."ReagentInfo" (
    "ReagentIdNum" bigint NOT NULL,
    "ReagentTypeNum" integer,
    "Current" boolean,
    "ContainerTagSN" character varying,
    "ReagentIndexList" smallint[],
    "ReagentNamesList" character varying[],
    "MixingCycles" smallint[],
    "PackPartNum" character varying,
    "LotNum" character varying,
    "LotExpiration" bigint,
    "InService" bigint,
    "ServiceLife" smallint,
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellInstrument"."ReagentInfo" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 259 (class 1259 OID 142574)
-- Name: ReagentInfo_ReagentIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."ReagentInfo_ReagentIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."ReagentInfo_ReagentIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3375 (class 0 OID 0)
-- Dependencies: 259
-- Name: ReagentInfo_ReagentIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."ReagentInfo_ReagentIdNum_seq" OWNED BY "ViCellInstrument"."ReagentInfo"."ReagentIdNum";


CREATE TABLE "ViCellInstrument"."CellHealthReagents" (
	"IdNum" bigint NOT NULL,
	"ID" uuid NOT NULL,
	"Type" smallint,
	"Name" character varying,
	"Volume" integer,
	"Protected" boolean DEFAULT true NOT NULL
);


ALTER TABLE "ViCellInstrument"."CellHealthReagents" OWNER TO "BCIViCellAdmin";


CREATE SEQUENCE "ViCellInstrument"."CellHealthReagents_IdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."CellHealthReagents_IdNum_seq" OWNER TO "BCIViCellAdmin";


ALTER SEQUENCE "ViCellInstrument"."CellHealthReagents_IdNum_seq" OWNED BY "ViCellInstrument"."CellHealthReagents"."IdNum";


--
-- TOC entry 260 (class 1259 OID 142576)
-- Name: Roles; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."Roles" (
    "RoleIdNum" bigint NOT NULL,
    "RoleID" uuid NOT NULL,
    "RoleName" character varying NOT NULL,
    "RoleType" smallint NOT NULL,
    "GroupMapList" character varying[],
    "CellTypeIndexList" integer[],
    "InstrumentPermissions" bigint,
    "ApplicationPermissions" bigint,
    "Protected" boolean DEFAULT true NOT NULL
);


ALTER TABLE "ViCellInstrument"."Roles" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 261 (class 1259 OID 142583)
-- Name: Roles_RoleIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."Roles_RoleIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."Roles_RoleIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3378 (class 0 OID 0)
-- Dependencies: 261
-- Name: Roles_RoleIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."Roles_RoleIdNum_seq" OWNED BY "ViCellInstrument"."Roles"."RoleIdNum";


--
-- TOC entry 262 (class 1259 OID 142585)
-- Name: SampleItems; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."SampleItems" (
    "SampleItemIdNum" bigint NOT NULL,
    "SampleItemID" uuid NOT NULL,
    "SampleItemStatus" smallint,
    "SampleItemName" character varying,
    "Comments" character varying,
    "RunDate" timestamp without time zone,
    "SampleSetID" uuid NOT NULL,
    "SampleID" uuid,
    "SaveImages" smallint,
    "WashType" smallint,
    "Dilution" smallint,
    "ItemLabel" character varying,
    "ImageAnalysisParamID" uuid,
    "AnalysisDefinitionID" uuid,
    "AnalysisDefinitionIndex" integer,
    "AnalysisParameterID" uuid,
    "CellTypeID" uuid,
    "CellTypeIndex" integer,
    "BioProcessID" uuid,
    "QcProcessID" uuid,
    "WorkflowID" uuid,
    "SamplePosition" character varying,
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellInstrument"."SampleItems" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 263 (class 1259 OID 142592)
-- Name: SampleItems_SampleItemIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."SampleItems_SampleItemIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."SampleItems_SampleItemIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3381 (class 0 OID 0)
-- Dependencies: 263
-- Name: SampleItems_SampleItemIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."SampleItems_SampleItemIdNum_seq" OWNED BY "ViCellInstrument"."SampleItems"."SampleItemIdNum";


--
-- TOC entry 264 (class 1259 OID 142594)
-- Name: SampleSets; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."SampleSets" (
    "SampleSetIdNum" bigint NOT NULL,
    "SampleSetID" uuid,
    "SampleSetStatus" smallint,
    "SampleSetName" character varying,
    "SampleSetLabel" character varying,
    "Comments" character varying,
    "CarrierType" smallint DEFAULT 0 NOT NULL,
    "OwnerID" uuid,
    "CreateDate" timestamp without time zone,
    "ModifyDate" timestamp without time zone,
    "RunDate" timestamp without time zone,
    "WorklistID" uuid,
    "SampleItemCount" smallint,
    "ProcessedItemCount" smallint,
    "SampleItemIDList" uuid[],
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellInstrument"."SampleSets" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 265 (class 1259 OID 142602)
-- Name: SampleSets_SampleSetIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."SampleSets_SampleSetIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."SampleSets_SampleSetIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3384 (class 0 OID 0)
-- Dependencies: 265
-- Name: SampleSets_SampleSetIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."SampleSets_SampleSetIdNum_seq" OWNED BY "ViCellInstrument"."SampleSets"."SampleSetIdNum";


--
-- TOC entry 279 (class 1259 OID 156906)
-- Name: Scheduler; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."Scheduler" (
    "SchedulerConfigIdNum" bigint NOT NULL,
    "SchedulerConfigID" uuid NOT NULL,
    "SchedulerName" character varying NOT NULL,
    "Comments" character varying,
    "OutputFilenameTemplate" character varying NOT NULL,
    "OwnerID" uuid NOT NULL,
    "CreationDate" timestamp without time zone NOT NULL,
    "OutputType" smallint DEFAULT 0 NOT NULL,
    "StartDate" timestamp without time zone,
    "StartOffset" smallint,
    "RepetitionInterval" integer DEFAULT 0 NOT NULL,
    "DayWeekIndicator" smallint DEFAULT 127 NOT NULL,
    "MonthlyRunDay" smallint DEFAULT 0 NOT NULL,
    "DestinationFolder" character varying NOT NULL,
    "DataType" integer,
    "FilterTypesList" integer[],
    "CompareOpsList" character varying[],
    "CompareValsList" character varying[],
    "Enabled" boolean DEFAULT false NOT NULL,
    "LastRunTime" timestamp without time zone,
    "LastSuccessRunTime" timestamp without time zone,
    "LastRunStatus" smallint DEFAULT 0 NOT NULL,
    "NotificationEmail" character varying,
    "EmailServer" character varying,
    "EmailServerPort" integer,
    "AuthenticateEmail" boolean,
    "EmailAccount" character varying,
    "EmailAccountAuthenticator" character varying,
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellInstrument"."Scheduler" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 278 (class 1259 OID 156904)
-- Name: Scheduler_SchedulerConfigIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."Scheduler_SchedulerConfigIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."Scheduler_SchedulerConfigIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3387 (class 0 OID 0)
-- Dependencies: 278
-- Name: Scheduler_SchedulerConfigIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."Scheduler_SchedulerConfigIdNum_seq" OWNED BY "ViCellInstrument"."Scheduler"."SchedulerConfigIdNum";


--
-- TOC entry 266 (class 1259 OID 142604)
-- Name: SignatureDefinitions; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."SignatureDefinitions" (
    "SignatureDefIdNum" bigint NOT NULL,
    "SignatureDefID" uuid NOT NULL,
    "ShortSignature" character varying,
    "ShortSignatureHash" character varying,
    "LongSignature" character varying,
    "LongSignatureHash" character varying,
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellInstrument"."SignatureDefinitions" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 267 (class 1259 OID 142611)
-- Name: SignatureDefinitions_SignatureDefIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."SignatureDefinitions_SignatureDefIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."SignatureDefinitions_SignatureDefIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3389 (class 0 OID 0)
-- Dependencies: 267
-- Name: SignatureDefinitions_SignatureDefIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."SignatureDefinitions_SignatureDefIdNum_seq" OWNED BY "ViCellInstrument"."SignatureDefinitions"."SignatureDefIdNum";


--
-- TOC entry 268 (class 1259 OID 142613)
-- Name: SystemLogs; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."SystemLogs" (
    "EntryIdNum" bigint NOT NULL,
    "EntryType" smallint NOT NULL,
    "EntryDate" timestamp without time zone NOT NULL,
    "EntryText" character varying NOT NULL,
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellInstrument"."SystemLogs" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 269 (class 1259 OID 142620)
-- Name: SystemLogs_EntryIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."SystemLogs_EntryIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."SystemLogs_EntryIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3392 (class 0 OID 0)
-- Dependencies: 269
-- Name: SystemLogs_EntryIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."SystemLogs_EntryIdNum_seq" OWNED BY "ViCellInstrument"."SystemLogs"."EntryIdNum";


--
-- TOC entry 270 (class 1259 OID 142622)
-- Name: UserProperties; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."UserProperties" (
    "PropertyIdNum" bigint NOT NULL,
    "PropertyIndex" smallint NOT NULL,
    "PropertyName" character varying NOT NULL,
    "PropertyType" smallint NOT NULL,
    "Protected" boolean DEFAULT true NOT NULL
);


ALTER TABLE "ViCellInstrument"."UserProperties" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 271 (class 1259 OID 142629)
-- Name: UserProperties_PropertyIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."UserProperties_PropertyIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."UserProperties_PropertyIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3395 (class 0 OID 0)
-- Dependencies: 271
-- Name: UserProperties_PropertyIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."UserProperties_PropertyIdNum_seq" OWNED BY "ViCellInstrument"."UserProperties"."PropertyIdNum";


--
-- TOC entry 272 (class 1259 OID 142631)
-- Name: Users; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."Users" (
    "UserIdNum" bigint NOT NULL,
    "UserID" uuid NOT NULL,
    "Retired" boolean DEFAULT false NOT NULL,
    "ADUser" boolean DEFAULT false NOT NULL,
    "RoleID" uuid NOT NULL,
    "UserName" character varying NOT NULL,
    "DisplayName" character varying,
    "Comments" character varying,
    "UserEmail" character varying,
    "AuthenticatorList" character varying[],
    "AuthenticatorDate" timestamp without time zone,
    "LastLogin" timestamp without time zone,
    "AttemptCount" smallint,
    "LanguageCode" character varying,
    "DefaultSampleName" character varying,
    "SaveNthIImage" smallint,
    "DisplayColumns" public.column_display_info[],
    "DecimalPrecision" smallint,
    "ExportFolder" character varying,
    "DefaultResultFileName" character varying,
    "CSVFolder" character varying,
    "PdfExport" boolean DEFAULT false NOT NULL,
    "AllowFastMode" boolean DEFAULT true NOT NULL,
    "WashType" smallint,
    "Dilution" smallint,
    "DefaultCellTypeIndex" integer,
    "NumUserCellTypes" smallint,
    "UserCellTypeIndexList" integer[],
    "UserAnalysisDefIndexList" integer[],
    "NumUserProperties" smallint,
    "UserPropertiesIndexList" smallint[],
    "AppPermissions" bigint,
    "AppPermissionsHash" character varying,
    "InstrumentPermissions" bigint,
    "InstrumentPermissionsHash" character varying,
    "Protected" boolean DEFAULT true NOT NULL
);


ALTER TABLE "ViCellInstrument"."Users" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 273 (class 1259 OID 142642)
-- Name: Users_UserIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."Users_UserIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."Users_UserIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3398 (class 0 OID 0)
-- Dependencies: 273
-- Name: Users_UserIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."Users_UserIdNum_seq" OWNED BY "ViCellInstrument"."Users"."UserIdNum";


--
-- TOC entry 274 (class 1259 OID 142644)
-- Name: Workflows; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."Workflows" (
    "WorkflowIdNum" bigint NOT NULL,
    "WorkflowID" uuid NOT NULL,
    "WorkflowName" character varying NOT NULL,
    "ReagentTypeList" integer[],
    "WorkflowSequenceControl" character varying NOT NULL,
    "Protected" boolean DEFAULT true NOT NULL
);


ALTER TABLE "ViCellInstrument"."Workflows" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 275 (class 1259 OID 142651)
-- Name: Workflows_WorkflowIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."Workflows_WorkflowIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."Workflows_WorkflowIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3401 (class 0 OID 0)
-- Dependencies: 275
-- Name: Workflows_WorkflowIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."Workflows_WorkflowIdNum_seq" OWNED BY "ViCellInstrument"."Workflows"."WorkflowIdNum";


--
-- TOC entry 276 (class 1259 OID 142653)
-- Name: Worklists; Type: TABLE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE TABLE "ViCellInstrument"."Worklists" (
    "WorklistIdNum" bigint NOT NULL,
    "WorklistID" uuid NOT NULL,
    "WorklistStatus" smallint,
    "WorklistName" character varying,
    "ListComments" character varying,
    "InstrumentSN" character varying,
    "CreationUserID" uuid NOT NULL,
    "RunUserID" uuid,
    "RunDate" timestamp without time zone,
    "AcquireSample" boolean,
    "CarrierType" smallint DEFAULT 0 NOT NULL,
    "ByColumn" boolean,
    "SaveImages" smallint NOT NULL,
    "WashType" smallint,
    "Dilution" smallint,
    "DefaultSetName" character varying,
    "DefaultItemName" character varying,
    "ImageAnalysisParamID" uuid,
    "AnalysisDefinitionID" uuid,
    "AnalysisDefinitionIndex" integer,
    "AnalysisParameterID" uuid,
    "CellTypeID" uuid,
    "CellTypeIndex" integer,
    "BioProcessID" uuid,
    "QcProcessID" uuid,
    "WorkflowID" uuid,
    "SampleSetCount" smallint,
    "ProcessedSetCount" smallint,
    "SampleSetIDList" uuid[],
    "Protected" boolean DEFAULT false NOT NULL
);


ALTER TABLE "ViCellInstrument"."Worklists" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 277 (class 1259 OID 142661)
-- Name: Worklists_WorklistIdNum_seq; Type: SEQUENCE; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

CREATE SEQUENCE "ViCellInstrument"."Worklists_WorklistIdNum_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE "ViCellInstrument"."Worklists_WorklistIdNum_seq" OWNER TO "BCIViCellAdmin";

--
-- TOC entry 3404 (class 0 OID 0)
-- Dependencies: 277
-- Name: Worklists_WorklistIdNum_seq; Type: SEQUENCE OWNED BY; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER SEQUENCE "ViCellInstrument"."Worklists_WorklistIdNum_seq" OWNED BY "ViCellInstrument"."Worklists"."WorklistIdNum";


--
-- TOC entry 2962 (class 2604 OID 142663)
-- Name: Analyses AnalysisIdNum; Type: DEFAULT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."Analyses" ALTER COLUMN "AnalysisIdNum" SET DEFAULT nextval('"ViCellData"."Analyses_AnalysisIdNum_seq"'::regclass);


--
-- TOC entry 2964 (class 2604 OID 142664)
-- Name: DetailedResults DetailedResultIdNum; Type: DEFAULT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."DetailedResults" ALTER COLUMN "DetailedResultIdNum" SET DEFAULT nextval('"ViCellData"."DetailedResults_DetailedResultIdNum_seq"'::regclass);


--
-- TOC entry 2966 (class 2604 OID 142665)
-- Name: ImageReferences ImageIdNum; Type: DEFAULT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."ImageReferences" ALTER COLUMN "ImageIdNum" SET DEFAULT nextval('"ViCellData"."ImageReferences_ImageIdNum_seq"'::regclass);


--
-- TOC entry 2968 (class 2604 OID 142666)
-- Name: ImageResults ResultIdNum; Type: DEFAULT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."ImageResults" ALTER COLUMN "ResultIdNum" SET DEFAULT nextval('"ViCellData"."ImageResults_ResultIdNum_seq"'::regclass);


--
-- TOC entry 2970 (class 2604 OID 142667)
-- Name: ImageSequences ImageSequenceIdNum; Type: DEFAULT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."ImageSequences" ALTER COLUMN "ImageSequenceIdNum" SET DEFAULT nextval('"ViCellData"."ImageSequences_ImageSequenceIdNum_seq"'::regclass);


--
-- TOC entry 2972 (class 2604 OID 142668)
-- Name: ImageSets ImageSetIdNum; Type: DEFAULT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."ImageSets" ALTER COLUMN "ImageSetIdNum" SET DEFAULT nextval('"ViCellData"."ImageSets_ImageSetIdNum_seq"'::regclass);


--
-- TOC entry 2974 (class 2604 OID 142669)
-- Name: SResults SResultIdNum; Type: DEFAULT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."SResults" ALTER COLUMN "SResultIdNum" SET DEFAULT nextval('"ViCellData"."SResults_SResultIdNum_seq"'::regclass);


--
-- TOC entry 2976 (class 2604 OID 142670)
-- Name: SampleProperties SampleIdNum; Type: DEFAULT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."SampleProperties" ALTER COLUMN "SampleIdNum" SET DEFAULT nextval('"ViCellData"."SampleProperties_SampleIdNum_seq"'::regclass);


--
-- TOC entry 2978 (class 2604 OID 142671)
-- Name: SummaryResults SummaryResultIdNum; Type: DEFAULT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."SummaryResults" ALTER COLUMN "SummaryResultIdNum" SET DEFAULT nextval('"ViCellData"."SummaryResults_SummaryResultIdNum_seq"'::regclass);


--
-- TOC entry 2981 (class 2604 OID 142672)
-- Name: AnalysisDefinitions AnalysisDefinitionIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."AnalysisDefinitions" ALTER COLUMN "AnalysisDefinitionIdNum" SET DEFAULT nextval('"ViCellInstrument"."AnalysisDefinitions_AnalysisDefinitionIdNum_seq"'::regclass);


--
-- TOC entry 2982 (class 2604 OID 142673)
-- Name: AnalysisInputSettings SettingsIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."AnalysisInputSettings" ALTER COLUMN "SettingsIdNum" SET DEFAULT nextval('"ViCellInstrument"."AnalysisInputSettings_SettingsIdNum_seq"'::regclass);


--
-- TOC entry 2984 (class 2604 OID 142674)
-- Name: AnalysisParams AnalysisParamIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."AnalysisParams" ALTER COLUMN "AnalysisParamIdNum" SET DEFAULT nextval('"ViCellInstrument"."AnalysisParams_AnalysisParamIdNum_seq"'::regclass);


--
-- TOC entry 2986 (class 2604 OID 142675)
-- Name: BioProcesses BioProcessIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."BioProcesses" ALTER COLUMN "BioProcessIdNum" SET DEFAULT nextval('"ViCellInstrument"."BioProcesses_BioProcessIdNum_seq"'::regclass);


--
-- TOC entry 2988 (class 2604 OID 142676)
-- Name: Calibrations CalibrationIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."Calibrations" ALTER COLUMN "CalibrationIdNum" SET DEFAULT nextval('"ViCellInstrument"."Calibrations_CalibrationIdNum_seq"'::regclass);


--
-- TOC entry 2992 (class 2604 OID 142677)
-- Name: CellTypes CellTypeIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."CellTypes" ALTER COLUMN "CellTypeIdNum" SET DEFAULT nextval('"ViCellInstrument"."CellTypes_CellTypeIdNum_seq"'::regclass);


--
-- TOC entry 2999 (class 2604 OID 142678)
-- Name: IlluminatorTypes IlluminatorIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."IlluminatorTypes" ALTER COLUMN "IlluminatorIdNum" SET DEFAULT nextval('"ViCellInstrument"."IlluminatorTypes_IlluminatorIdNum_seq"'::regclass);


--
-- TOC entry 3001 (class 2604 OID 142679)
-- Name: ImageAnalysisCellIdentParams IdentParamIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."ImageAnalysisCellIdentParams" ALTER COLUMN "IdentParamIdNum" SET DEFAULT nextval('"ViCellInstrument"."ImageAnalysisCellIdentParams_IdentParamIdNum_seq"'::regclass);


--
-- TOC entry 3003 (class 2604 OID 142680)
-- Name: ImageAnalysisParams ImageAnalysisParamIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."ImageAnalysisParams" ALTER COLUMN "ImageAnalysisParamIdNum" SET DEFAULT nextval('"ViCellInstrument"."ImageAnalysisParams_ImageAnalysisParamIdNum_seq"'::regclass);


--
-- TOC entry 3020 (class 2604 OID 142681)
-- Name: InstrumentConfig InstrumentIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."InstrumentConfig" ALTER COLUMN "InstrumentIdNum" SET DEFAULT nextval('"ViCellInstrument"."InstrumentConfig_InstrumentIdNum_seq"'::regclass);


--
-- TOC entry 3022 (class 2604 OID 142682)
-- Name: QcProcesses QcIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."QcProcesses" ALTER COLUMN "QcIdNum" SET DEFAULT nextval('"ViCellInstrument"."QcProcesses_QcIdNum_seq"'::regclass);


--
-- TOC entry 3024 (class 2604 OID 142683)
-- Name: ReagentInfo ReagentIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."ReagentInfo" ALTER COLUMN "ReagentIdNum" SET DEFAULT nextval('"ViCellInstrument"."ReagentInfo_ReagentIdNum_seq"'::regclass);


--


ALTER TABLE ONLY "ViCellInstrument"."CellHealthReagents" ALTER COLUMN "IdNum" SET DEFAULT nextval('"ViCellInstrument"."CellHealthReagents_IdNum_seq"'::regclass);


--
-- TOC entry 3026 (class 2604 OID 142684)
-- Name: Roles RoleIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."Roles" ALTER COLUMN "RoleIdNum" SET DEFAULT nextval('"ViCellInstrument"."Roles_RoleIdNum_seq"'::regclass);


--
-- TOC entry 3028 (class 2604 OID 142685)
-- Name: SampleItems SampleItemIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."SampleItems" ALTER COLUMN "SampleItemIdNum" SET DEFAULT nextval('"ViCellInstrument"."SampleItems_SampleItemIdNum_seq"'::regclass);


--
-- TOC entry 3031 (class 2604 OID 142686)
-- Name: SampleSets SampleSetIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."SampleSets" ALTER COLUMN "SampleSetIdNum" SET DEFAULT nextval('"ViCellInstrument"."SampleSets_SampleSetIdNum_seq"'::regclass);


--
-- TOC entry 3049 (class 2604 OID 156909)
-- Name: Scheduler SchedulerConfigIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."Scheduler" ALTER COLUMN "SchedulerConfigIdNum" SET DEFAULT nextval('"ViCellInstrument"."Scheduler_SchedulerConfigIdNum_seq"'::regclass);


--
-- TOC entry 3033 (class 2604 OID 142687)
-- Name: SignatureDefinitions SignatureDefIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."SignatureDefinitions" ALTER COLUMN "SignatureDefIdNum" SET DEFAULT nextval('"ViCellInstrument"."SignatureDefinitions_SignatureDefIdNum_seq"'::regclass);


--
-- TOC entry 3035 (class 2604 OID 142688)
-- Name: SystemLogs EntryIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."SystemLogs" ALTER COLUMN "EntryIdNum" SET DEFAULT nextval('"ViCellInstrument"."SystemLogs_EntryIdNum_seq"'::regclass);


--
-- TOC entry 3037 (class 2604 OID 142689)
-- Name: UserProperties PropertyIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."UserProperties" ALTER COLUMN "PropertyIdNum" SET DEFAULT nextval('"ViCellInstrument"."UserProperties_PropertyIdNum_seq"'::regclass);


--
-- TOC entry 3043 (class 2604 OID 142690)
-- Name: Users UserIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."Users" ALTER COLUMN "UserIdNum" SET DEFAULT nextval('"ViCellInstrument"."Users_UserIdNum_seq"'::regclass);


--
-- TOC entry 3045 (class 2604 OID 142691)
-- Name: Workflows WorkflowIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."Workflows" ALTER COLUMN "WorkflowIdNum" SET DEFAULT nextval('"ViCellInstrument"."Workflows_WorkflowIdNum_seq"'::regclass);


--
-- TOC entry 3048 (class 2604 OID 142692)
-- Name: Worklists WorklistIdNum; Type: DEFAULT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."Worklists" ALTER COLUMN "WorklistIdNum" SET DEFAULT nextval('"ViCellInstrument"."Worklists_WorklistIdNum_seq"'::regclass);


--
-- TOC entry 3238 (class 0 OID 142374)
-- Dependencies: 218
-- Data for Name: Analyses; Type: TABLE DATA; Schema: ViCellData; Owner: BCIViCellAdmin
--



--
-- TOC entry 3240 (class 0 OID 142383)
-- Dependencies: 220
-- Data for Name: DetailedResults; Type: TABLE DATA; Schema: ViCellData; Owner: BCIViCellAdmin
--



--
-- TOC entry 3242 (class 0 OID 142389)
-- Dependencies: 222
-- Data for Name: ImageReferences; Type: TABLE DATA; Schema: ViCellData; Owner: BCIViCellAdmin
--



--
-- TOC entry 3244 (class 0 OID 142398)
-- Dependencies: 224
-- Data for Name: ImageResults; Type: TABLE DATA; Schema: ViCellData; Owner: BCIViCellAdmin
--



--
-- TOC entry 3246 (class 0 OID 142407)
-- Dependencies: 226
-- Data for Name: ImageSequences; Type: TABLE DATA; Schema: ViCellData; Owner: BCIViCellAdmin
--



--
-- TOC entry 3248 (class 0 OID 142416)
-- Dependencies: 228
-- Data for Name: ImageSets; Type: TABLE DATA; Schema: ViCellData; Owner: BCIViCellAdmin
--



--
-- TOC entry 3250 (class 0 OID 142425)
-- Dependencies: 230
-- Data for Name: SResults; Type: TABLE DATA; Schema: ViCellData; Owner: BCIViCellAdmin
--



--
-- TOC entry 3252 (class 0 OID 142434)
-- Dependencies: 232
-- Data for Name: SampleProperties; Type: TABLE DATA; Schema: ViCellData; Owner: BCIViCellAdmin
--



--
-- TOC entry 3254 (class 0 OID 142443)
-- Dependencies: 234
-- Data for Name: SummaryResults; Type: TABLE DATA; Schema: ViCellData; Owner: BCIViCellAdmin
--



--
-- TOC entry 3256 (class 0 OID 142452)
-- Dependencies: 236
-- Data for Name: AnalysisDefinitions; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

INSERT INTO "ViCellInstrument"."AnalysisDefinitions" ("AnalysisDefinitionIdNum", "AnalysisDefinitionID", "AnalysisDefinitionIndex", "AnalysisDefinitionName", "NumReagents", "ReagentTypeIndexList", "MixingCycles", "NumIlluminators", "IlluminatorsIndexList", "NumAnalysisParams", "AnalysisParamIDList", "PopulationParamExists", "PopulationParamID", "Protected") VALUES (1, '685c3b26-807f-4303-a890-ec86f6be6f7a', 0, 'Viable (TB)', 1, '{4}', 3, 0, '{}', 2, '{eea18195-2002-4e53-bfb7-f4ccbed6b2c7,ab64c2e2-a4b4-47b3-b40b-551f80f9b1b1}', false, NULL, true);
INSERT INTO "ViCellInstrument"."AnalysisDefinitions" ("AnalysisDefinitionIdNum", "AnalysisDefinitionID", "AnalysisDefinitionIndex", "AnalysisDefinitionName", "NumReagents", "ReagentTypeIndexList", "MixingCycles", "NumIlluminators", "IlluminatorsIndexList", "NumAnalysisParams", "AnalysisParamIDList", "PopulationParamExists", "PopulationParamID", "Protected") VALUES (2, '17615cb6-a557-4cda-88fd-0452421daa03', 0, 'Viable (TB)', 1, '{4}', 3, 0, '{}', 2, '{8ca1d65f-cdba-48e5-9a15-e0ab99a55fc9,db37a5b9-c8c3-44ca-b840-1913bb9e268e}', false, NULL, true);
INSERT INTO "ViCellInstrument"."AnalysisDefinitions" ("AnalysisDefinitionIdNum", "AnalysisDefinitionID", "AnalysisDefinitionIndex", "AnalysisDefinitionName", "NumReagents", "ReagentTypeIndexList", "MixingCycles", "NumIlluminators", "IlluminatorsIndexList", "NumAnalysisParams", "AnalysisParamIDList", "PopulationParamExists", "PopulationParamID", "Protected") VALUES (3, 'af21144c-b7ed-4445-b709-ecfad10ba38f', 0, 'Viable (TB)', 1, '{4}', 3, 0, '{}', 2, '{e259149a-b3f3-4365-8920-902ca9a39c67,0e4399bd-c7ff-4c03-8ae7-a3c27e0a3221}', false, NULL, true);
INSERT INTO "ViCellInstrument"."AnalysisDefinitions" ("AnalysisDefinitionIdNum", "AnalysisDefinitionID", "AnalysisDefinitionIndex", "AnalysisDefinitionName", "NumReagents", "ReagentTypeIndexList", "MixingCycles", "NumIlluminators", "IlluminatorsIndexList", "NumAnalysisParams", "AnalysisParamIDList", "PopulationParamExists", "PopulationParamID", "Protected") VALUES (4, '3f4d3a35-20bd-41ad-999a-beec31095464', 0, 'Viable (TB)', 1, '{4}', 3, 0, '{}', 2, '{cec1d6e2-ed5b-40d2-96e7-b78052417792,0c0589d1-3d0c-4a69-bac5-36b1e9b91ba9}', false, NULL, true);
INSERT INTO "ViCellInstrument"."AnalysisDefinitions" ("AnalysisDefinitionIdNum", "AnalysisDefinitionID", "AnalysisDefinitionIndex", "AnalysisDefinitionName", "NumReagents", "ReagentTypeIndexList", "MixingCycles", "NumIlluminators", "IlluminatorsIndexList", "NumAnalysisParams", "AnalysisParamIDList", "PopulationParamExists", "PopulationParamID", "Protected") VALUES (5, 'fba1bfb9-a30b-4a98-9571-0fe2ca5053a7', 0, 'Viable (TB)', 1, '{4}', 3, 0, '{}', 2, '{fc2bdb46-c148-4687-8d51-88a5f1c1575b,bbcb75ad-48d3-4ad5-b2b7-ac795d5e7f5d}', false, NULL, true);
INSERT INTO "ViCellInstrument"."AnalysisDefinitions" ("AnalysisDefinitionIdNum", "AnalysisDefinitionID", "AnalysisDefinitionIndex", "AnalysisDefinitionName", "NumReagents", "ReagentTypeIndexList", "MixingCycles", "NumIlluminators", "IlluminatorsIndexList", "NumAnalysisParams", "AnalysisParamIDList", "PopulationParamExists", "PopulationParamID", "Protected") VALUES (6, 'f36b4017-0f6c-4a59-afff-f21b912d40ac', 0, 'Viable (TB)', 1, '{4}', 3, 0, '{}', 2, '{639a2281-77f3-4b5b-97b6-5f0237d138d5,22b1866b-bc0f-49af-92e9-7e948a56106e}', false, NULL, true);
INSERT INTO "ViCellInstrument"."AnalysisDefinitions" ("AnalysisDefinitionIdNum", "AnalysisDefinitionID", "AnalysisDefinitionIndex", "AnalysisDefinitionName", "NumReagents", "ReagentTypeIndexList", "MixingCycles", "NumIlluminators", "IlluminatorsIndexList", "NumAnalysisParams", "AnalysisParamIDList", "PopulationParamExists", "PopulationParamID", "Protected") VALUES (7, '9f0c7de5-489c-4c0f-8747-56846cabb9d8', 0, 'Viable (TB)', 1, '{4}', 3, 0, '{}', 2, '{ced3c4b5-4248-4d1f-95b7-fb2d247002da,23a4929f-1835-4b21-a4b4-ae060b0b684e}', false, NULL, true);
INSERT INTO "ViCellInstrument"."AnalysisDefinitions" ("AnalysisDefinitionIdNum", "AnalysisDefinitionID", "AnalysisDefinitionIndex", "AnalysisDefinitionName", "NumReagents", "ReagentTypeIndexList", "MixingCycles", "NumIlluminators", "IlluminatorsIndexList", "NumAnalysisParams", "AnalysisParamIDList", "PopulationParamExists", "PopulationParamID", "Protected") VALUES (8, '4672bd76-a7ec-4a38-9457-72193392e923', 0, 'Viable (TB)', 1, '{4}', 3, 0, '{}', 2, '{27532029-f12c-4b49-99b0-25c504edaa0c,1fa1368f-d6b6-4281-92ad-ee3c39920603}', false, NULL, true);


--
-- TOC entry 3258 (class 0 OID 142462)
-- Dependencies: 238
-- Data for Name: AnalysisInputSettings; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

INSERT INTO "ViCellInstrument"."AnalysisInputSettings" ("SettingsIdNum", "SettingsID", "InputConfigParamMap", "CellIdentParamList", "POIIdentParamList") VALUES (1, 'e0e448df-94c5-421f-8224-17a7f2e6b053', '{"(0,1)","(1,1)","(2,1)","(3,0)","(4,1)","(5,0.53000000000000003)","(6,-5.5300000000000002)","(8,60)","(9,60)","(10,16)","(11,18)","(13,368)","(14,0)","(15,100)","(16,5)","(17,30)","(18,30)","(19,53)","(20,8)","(21,50)","(22,50)","(23,1)","(24,0)","(25,1)","(26,19)"}', '{"(8,0,0,8,1)","(8,0,0,50,0)","(9,0,0,0.100000001,1)","(10,0,0,7,1)"}', '{"(20,0,0,5,1)","(21,0,0,50,1)"}');
INSERT INTO "ViCellInstrument"."AnalysisInputSettings" ("SettingsIdNum", "SettingsID", "InputConfigParamMap", "CellIdentParamList", "POIIdentParamList") VALUES (2, '20a9f393-1f5c-440f-ba33-011ff47ef450', '{"(0,1)","(1,1)","(2,1)","(3,0)","(4,1)","(5,0.53000000000000003)","(6,-5.5300000000000002)","(8,60)","(9,60)","(10,16)","(11,18)","(13,368)","(14,0)","(15,100)","(16,5)","(17,30)","(18,30)","(19,53)","(20,8)","(21,50)","(22,50)","(23,1)","(24,0)","(25,1)","(26,19)"}', '{"(8,0,0,8,1)","(8,0,0,50,0)","(9,0,0,0.100000001,1)","(10,0,0,7,1)"}', '{"(20,0,0,5,1)","(21,0,0,55,1)"}');


--
-- TOC entry 3260 (class 0 OID 142470)
-- Dependencies: 240
-- Data for Name: AnalysisParams; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

INSERT INTO "ViCellInstrument"."AnalysisParams" ("AnalysisParamIdNum", "AnalysisParamID", "IsInitialized", "AnalysisParamLabel", "CharacteristicKey", "CharacteristicSKey", "CharacteristicSSKey", "ThresholdValue", "AboveThreshold", "Protected") VALUES (1, 'eea18195-2002-4e53-bfb7-f4ccbed6b2c7', false, 'Cell Spot Area', 20, 0, 0, 5, true, true);
INSERT INTO "ViCellInstrument"."AnalysisParams" ("AnalysisParamIdNum", "AnalysisParamID", "IsInitialized", "AnalysisParamLabel", "CharacteristicKey", "CharacteristicSKey", "CharacteristicSSKey", "ThresholdValue", "AboveThreshold", "Protected") VALUES (2, 'ab64c2e2-a4b4-47b3-b40b-551f80f9b1b1', false, 'Average Spot Brightness', 21, 0, 0, 50, true, true);
INSERT INTO "ViCellInstrument"."AnalysisParams" ("AnalysisParamIdNum", "AnalysisParamID", "IsInitialized", "AnalysisParamLabel", "CharacteristicKey", "CharacteristicSKey", "CharacteristicSSKey", "ThresholdValue", "AboveThreshold", "Protected") VALUES (3, '8ca1d65f-cdba-48e5-9a15-e0ab99a55fc9', false, 'Cell Spot Area', 20, 0, 0, 1, true, true);
INSERT INTO "ViCellInstrument"."AnalysisParams" ("AnalysisParamIdNum", "AnalysisParamID", "IsInitialized", "AnalysisParamLabel", "CharacteristicKey", "CharacteristicSKey", "CharacteristicSSKey", "ThresholdValue", "AboveThreshold", "Protected") VALUES (4, 'db37a5b9-c8c3-44ca-b840-1913bb9e268e', false, 'Average Spot Brightness', 21, 0, 0, 50, true, true);
INSERT INTO "ViCellInstrument"."AnalysisParams" ("AnalysisParamIdNum", "AnalysisParamID", "IsInitialized", "AnalysisParamLabel", "CharacteristicKey", "CharacteristicSKey", "CharacteristicSSKey", "ThresholdValue", "AboveThreshold", "Protected") VALUES (5, 'e259149a-b3f3-4365-8920-902ca9a39c67', false, 'Cell Spot Area', 20, 0, 0, 5, true, true);
INSERT INTO "ViCellInstrument"."AnalysisParams" ("AnalysisParamIdNum", "AnalysisParamID", "IsInitialized", "AnalysisParamLabel", "CharacteristicKey", "CharacteristicSKey", "CharacteristicSSKey", "ThresholdValue", "AboveThreshold", "Protected") VALUES (6, '0e4399bd-c7ff-4c03-8ae7-a3c27e0a3221', false, 'Average Spot Brightness', 21, 0, 0, 55, true, true);
INSERT INTO "ViCellInstrument"."AnalysisParams" ("AnalysisParamIdNum", "AnalysisParamID", "IsInitialized", "AnalysisParamLabel", "CharacteristicKey", "CharacteristicSKey", "CharacteristicSSKey", "ThresholdValue", "AboveThreshold", "Protected") VALUES (7, 'cec1d6e2-ed5b-40d2-96e7-b78052417792', false, 'Cell Spot Area', 20, 0, 0, 5, true, true);
INSERT INTO "ViCellInstrument"."AnalysisParams" ("AnalysisParamIdNum", "AnalysisParamID", "IsInitialized", "AnalysisParamLabel", "CharacteristicKey", "CharacteristicSKey", "CharacteristicSSKey", "ThresholdValue", "AboveThreshold", "Protected") VALUES (8, '0c0589d1-3d0c-4a69-bac5-36b1e9b91ba9', false, 'Average Spot Brightness', 21, 0, 0, 55, true, true);
INSERT INTO "ViCellInstrument"."AnalysisParams" ("AnalysisParamIdNum", "AnalysisParamID", "IsInitialized", "AnalysisParamLabel", "CharacteristicKey", "CharacteristicSKey", "CharacteristicSSKey", "ThresholdValue", "AboveThreshold", "Protected") VALUES (9, 'fc2bdb46-c148-4687-8d51-88a5f1c1575b', false, 'Cell Spot Area', 20, 0, 0, 5, true, true);
INSERT INTO "ViCellInstrument"."AnalysisParams" ("AnalysisParamIdNum", "AnalysisParamID", "IsInitialized", "AnalysisParamLabel", "CharacteristicKey", "CharacteristicSKey", "CharacteristicSSKey", "ThresholdValue", "AboveThreshold", "Protected") VALUES (10, 'bbcb75ad-48d3-4ad5-b2b7-ac795d5e7f5d', false, 'Average Spot Brightness', 21, 0, 0, 55, true, true);
INSERT INTO "ViCellInstrument"."AnalysisParams" ("AnalysisParamIdNum", "AnalysisParamID", "IsInitialized", "AnalysisParamLabel", "CharacteristicKey", "CharacteristicSKey", "CharacteristicSSKey", "ThresholdValue", "AboveThreshold", "Protected") VALUES (11, '639a2281-77f3-4b5b-97b6-5f0237d138d5', false, 'Cell Spot Area', 20, 0, 0, 2, true, true);
INSERT INTO "ViCellInstrument"."AnalysisParams" ("AnalysisParamIdNum", "AnalysisParamID", "IsInitialized", "AnalysisParamLabel", "CharacteristicKey", "CharacteristicSKey", "CharacteristicSSKey", "ThresholdValue", "AboveThreshold", "Protected") VALUES (12, '22b1866b-bc0f-49af-92e9-7e948a56106e', false, 'Average Spot Brightness', 21, 0, 0, 45, true, true);
INSERT INTO "ViCellInstrument"."AnalysisParams" ("AnalysisParamIdNum", "AnalysisParamID", "IsInitialized", "AnalysisParamLabel", "CharacteristicKey", "CharacteristicSKey", "CharacteristicSSKey", "ThresholdValue", "AboveThreshold", "Protected") VALUES (13, 'ced3c4b5-4248-4d1f-95b7-fb2d247002da', false, 'Cell Spot Area', 20, 0, 0, 1, true, true);
INSERT INTO "ViCellInstrument"."AnalysisParams" ("AnalysisParamIdNum", "AnalysisParamID", "IsInitialized", "AnalysisParamLabel", "CharacteristicKey", "CharacteristicSKey", "CharacteristicSSKey", "ThresholdValue", "AboveThreshold", "Protected") VALUES (14, '23a4929f-1835-4b21-a4b4-ae060b0b684e', false, 'Average Spot Brightness', 21, 0, 0, 50, true, true);
INSERT INTO "ViCellInstrument"."AnalysisParams" ("AnalysisParamIdNum", "AnalysisParamID", "IsInitialized", "AnalysisParamLabel", "CharacteristicKey", "CharacteristicSKey", "CharacteristicSSKey", "ThresholdValue", "AboveThreshold", "Protected") VALUES (15, '27532029-f12c-4b49-99b0-25c504edaa0c', false, 'Cell Spot Area', 20, 0, 0, 1, true, true);
INSERT INTO "ViCellInstrument"."AnalysisParams" ("AnalysisParamIdNum", "AnalysisParamID", "IsInitialized", "AnalysisParamLabel", "CharacteristicKey", "CharacteristicSKey", "CharacteristicSSKey", "ThresholdValue", "AboveThreshold", "Protected") VALUES (16, '1fa1368f-d6b6-4281-92ad-ee3c39920603', false, 'Average Spot Brightness', 21, 0, 0, 50, true, true);


--
-- TOC entry 3262 (class 0 OID 142479)
-- Dependencies: 242
-- Data for Name: BioProcesses; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--



--
-- TOC entry 3264 (class 0 OID 142488)
-- Dependencies: 244
-- Data for Name: Calibrations; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

INSERT INTO "ViCellInstrument"."Calibrations" ("CalibrationIdNum", "CalibrationID", "InstrumentSN", "CalibrationDate", "CalibrationUserID", "CalibrationType", "Slope", "Intercept", "ImageCount", "CalQueueID", "ConsumablesList", "Protected") VALUES (1, '3e477819-a004-4ada-8137-142c5e05ccbe', '', '2018-01-01 00:00:00', '68cd72e5-1200-4f17-ab23-832b8884c69e', 1, 368, 0, 100, '00000000-0000-0000-0000-000000000000', '{"(\"Factory Default\",n/a,0,\"2100-01-01 00:00:00\",2000000)","(\"Factory Default\",n/a,0,\"2100-01-01 00:00:00\",4000000)","(\"Factory Default\",n/a,0,\"2100-01-01 00:00:00\",10000000)"}', true);
INSERT INTO "ViCellInstrument"."Calibrations" ("CalibrationIdNum", "CalibrationID", "InstrumentSN", "CalibrationDate", "CalibrationUserID", "CalibrationType", "Slope", "Intercept", "ImageCount", "CalQueueID", "ConsumablesList", "Protected") VALUES (2, '8e4456cb-77e0-4d4d-bd16-cdbd7d71b1e4', '', '2018-01-01 00:00:00', '68cd72e5-1200-4f17-ab23-832b8884c69e', 2, 0.53, -5.53, 0, '00000000-0000-0000-0000-000000000000', '{"(\"Factory Default\",n/a,0,\"2100-01-01 00:00:00\",2000000)","(\"Factory Default\",n/a,0,\"2100-01-01 00:00:00\",4000000)","(\"Factory Default\",n/a,0,\"2100-01-01 00:00:00\",10000000)"}', true);


--
-- TOC entry 3266 (class 0 OID 142497)
-- Dependencies: 246
-- Data for Name: CellTypes; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeIdNum", "CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES (1, '4a174b37-926a-462d-a165-f7bd2c70fac7', 0, 'BCI Default', false, 100, 3, 1, 50, 0.100000001, 7, 0, '{}', 2, 0, 60, 60, 1, '{af21144c-b7ed-4445-b709-ecfad10ba38f}', 0, true);
INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeIdNum", "CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES (2, 'cb57cb2f-2448-46e5-a33e-2987e3d38860', 1, 'Mammalian', false, 100, 3, 6, 30, 0.100000001, 7, 0, '{}', 2, 0, 60, 60, 1, '{3f4d3a35-20bd-41ad-999a-beec31095464}', 0, true);
INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeIdNum", "CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES (3, 'e3fe7d7c-1d09-43b1-b13f-41b5295b352a', 2, 'Insect', false, 100, 3, 8, 50, 0.100000001, 7, 0, '{}', 2, 0, 60, 60, 1, '{fba1bfb9-a30b-4a98-9571-0fe2ca5053a7}', 0, true);
INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeIdNum", "CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES (4, '450fa2e0-2077-4d3d-bb35-f85a8f60fe62', 3, 'Yeast', false, 100, 3, 3, 20, 0.100000001, 4, 0, '{}', 3, 0, 60, 60, 1, '{f36b4017-0f6c-4a59-afff-f21b912d40ac}', 0, true);
INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeIdNum", "CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES (5, 'c3b5dbbe-feae-4cc7-8a96-c01dbd691e50', 4, 'BCI Viab Beads', false, 100, 3, 5, 25, 0.5, 22, 0, '{}', 2, 0, 60, 60, 1, '{9f0c7de5-489c-4c0f-8747-56846cabb9d8}', 0, true);
INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeIdNum", "CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES (6, '568b387f-1004-477e-8267-f01329f2e780', 5, 'BCI Conc Beads', false, 100, 3, 2.5, 12, 0.75, 17, 0, '{}', 3, 0, 60, 60, 1, '{17615cb6-a557-4cda-88fd-0452421daa03}', 0, true);
INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeIdNum", "CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES (7, 'f207e97c-851a-42e1-8c29-38721cb15d90', 6, 'BCI L10 Beads', false, 100, 3, 5, 15, 0.5, 22, 0, '{}', 2, 0, 60, 60, 1, '{4672bd76-a7ec-4a38-9457-72193392e923}', 0, true);

INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeIdNum", "CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES (8, '45b743be-abe3-436e-aab1-99bee2a7d404', 7, 'BCI Default_LowCellDensity', false, 250, 3, 1, 50, 0.100000001, 7, 0, '{}', 2, 0, 60, 60, 1, '{af21144c-b7ed-4445-b709-ecfad10ba38f}', 0, true);
INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeIdNum", "CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES (9, 'cf3720e3-a6e8-4ff6-a72d-cd433b92c92a', 8, 'Mammalian_LowCellDensity', false, 250, 3, 6, 30, 0.100000001, 7, 0, '{}', 2, 0, 60, 60, 1, '{3f4d3a35-20bd-41ad-999a-beec31095464}', 0, true);
INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeIdNum", "CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES (10, '2b397692-eb04-4a21-98a2-8f9623bc011e', 9, 'Insect_LowCellDensity', false, 250, 3, 8, 50, 0.100000001, 7, 0, '{}', 2, 0, 60, 60, 1, '{fba1bfb9-a30b-4a98-9571-0fe2ca5053a7}', 0, true);
INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeIdNum", "CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES (11, '96f826c2-890d-4fe3-a5dd-7884f45214d5', 10, 'Yeast_LowCellDensity', false, 250, 3, 3, 20, 0.100000001, 4, 0, '{}', 3, 0, 60, 60, 1, '{f36b4017-0f6c-4a59-afff-f21b912d40ac}', 0, true);
INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeIdNum", "CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES (12, '2adff531-ea8c-41c8-8e89-7acd9c23692a', 11, 'BCI Viab Beads_LowCellDensity', false, 250, 3, 5, 25, 0.5, 22, 0, '{}', 2, 0, 60, 60, 1, '{9f0c7de5-489c-4c0f-8747-56846cabb9d8}', 0, true);
INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeIdNum", "CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES (13, 'c5516ff5-ad29-407c-9d1f-9a2090827374', 12, 'BCI Conc Beads_LowCellDensity', false, 250, 3, 2.5, 12, 0.75, 17, 0, '{}', 3, 0, 60, 60, 1, '{17615cb6-a557-4cda-88fd-0452421daa03}', 0, true);
INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeIdNum", "CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES (14, 'ef8d7357-9d67-4154-aa6e-5e915ed251a5', 13, 'BCI L10 Beads_LowCellDensity', false, 250, 3, 5, 15, 0.5, 22, 0, '{}', 2, 0, 60, 60, 1, '{4672bd76-a7ec-4a38-9457-72193392e923}', 0, true);


--
-- TOC entry 3268 (class 0 OID 142508)
-- Dependencies: 248
-- Data for Name: IlluminatorTypes; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

INSERT INTO "ViCellInstrument"."IlluminatorTypes" ("IlluminatorIdNum", "IlluminatorIndex", "IlluminatorType", "IlluminatorName", "PositionNum", "Tolerance", "MaxVoltage", "IlluminatorWavelength", "EmissionWavelength", "ExposureTimeMs", "PercentPower", "SimmerVoltage", "Ltcd", "Ctld", "FeedbackPhotoDiode", "Protected") VALUES (1, 0, 1, 'BRIGHTFIELD', 1, 0.100000001, 1500000, 999, 0, 0, 20, 0, 100, 100, 1, true);
INSERT INTO "ViCellInstrument"."IlluminatorTypes" ("IlluminatorIdNum", "IlluminatorIndex", "IlluminatorType", "IlluminatorName", "PositionNum", "Tolerance", "MaxVoltage", "IlluminatorWavelength", "EmissionWavelength", "ExposureTimeMs", "PercentPower", "SimmerVoltage", "Ltcd", "Ctld", "FeedbackPhotoDiode", "Protected") VALUES (2, 1, 2, 'BRIGHTFIELD', 1, 0.100000001, 1000000, 999, 0, 0, 20, 0, 700, 0, 1, true);


--
-- TOC entry 3270 (class 0 OID 142522)
-- Dependencies: 250
-- Data for Name: ImageAnalysisCellIdentParams; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--



--
-- TOC entry 3272 (class 0 OID 142528)
-- Dependencies: 252
-- Data for Name: ImageAnalysisParams; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

INSERT INTO "ViCellInstrument"."ImageAnalysisParams" ("ImageAnalysisParamIdNum", "ImageAnalysisParamID", "AlgorithmMode", "BubbleMode", "DeclusterMode", "SubPeakAnalysisMode", "DilutionFactor", "ROIXcoords", "ROIYcoords", "DeclusterAccumulatorThreshLow", "DeclusterMinDistanceThreshLow", "DeclusterAccumulatorThreshMed", "DeclusterMinDistanceThreshMed", "DeclusterAccumulatorThreshHigh", "DeclusterMinDistanceThreshHigh", "FovDepthMM", "PixelFovMM", "SizingSlope", "SizingIntercept", "ConcSlope", "ConcIntercept", "ConcImageControlCnt", "BubbleMinSpotAreaPrcnt", "BubbleMinSpotAreaBrightness", "BubbleRejectImgAreaPrcnt", "VisibleCellSpotArea", "FlScalableROI", "FLPeakPercent", "NominalBkgdLevel", "BkgdIntensityTolerance", "CenterSpotMinIntensityLimit", "PeakIntensitySelectionAreaLimit", "CellSpotBrightnessExclusionThreshold", "HotPixelEliminationMode", "ImgBotAndRightBoundaryAnnotationMode", "SmallParticleSizingCorrection", "Protected") VALUES (1, '49c3a5b7-7e73-4cb8-960c-3a9b420d04ad', 1, true, false, false, 0, 60, 60, 20, 22, 16, 18, 12, 15, 0.085999999999999993, 0.00048000000000000001, 0, 0, 0, 0, 100, 5, 30, 30, 0, 0, 0, 53, 8, 50, 50, 1, 0, 0, 19, true);


--
-- TOC entry 3274 (class 0 OID 142534)
-- Dependencies: 254
-- Data for Name: InstrumentConfig; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

INSERT INTO "ViCellInstrument"."InstrumentConfig" ("InstrumentIdNum", "InstrumentSN", "InstrumentType", "DeviceName", "UIVersion", "SoftwareVersion", "AnalysisSWVersion", "FirmwareVersion", "BrightFieldLedType", "CameraType", "CameraFWVersion", "CameraConfig", "PumpType", "PumpFWVersion", "PumpConfig", "IlluminatorsInfoList", "IlluminatorConfig", "ConfigType", "LogName", "LogMaxSize", "LogSensitivity", "MaxLogs", "AlwaysFlush", "CameraErrorLogName", "CameraErrorLogMaxSize", "StorageErrorLogName", "StorageErrorLogMaxSize", "CarouselThetaHomeOffset", "CarouselRadiusOffset", "PlateThetaHomePosOffset", "PlateThetaCalPos", "PlateRadiusCenterPos", "SaveImage", "FocusPosition", "AutoFocus", "AbiMaxImageCount", "SampleNudgeVolume", "SampleNudgeSpeed", "FlowCellDepth", "FlowCellDepthConstant", "RfidSim", "LegacyData", "CarouselSimulator", "NightlyCleanOffset", "LastNightlyClean", "SecurityMode", "InactivityTimeout", "PasswordExpiration", "NormalShutdown", "NextAnalysisDefIndex", "NextFactoryCellTypeIndex", "NextUserCellTypeIndex", "SamplesProcessed", "DiscardCapacity", "EmailServer", "ADSettings", "LanguageList", "DefaultDisplayColumns", "RunOptionDefaults", "AutomationInstalled", "AutomationEnabled", "ACupEnabled", "AutomationPort", "Protected") VALUES (1, 'InstrumentDefault', 3, 'CellHealth Science Module', '', '', '', '', 1, 1, '', '', 1, '', '', '{"(1,0)"}', '', 0, 'HawkeyeDLL.log', 10000000, 'DBG1', 25, true, 'CameraErrorLogger.log', 1000000, 'StorageErrorLogger.log', 1000000, 0, 0, 0, 0, 0, 1, 0, '(t,45000,75000,300,2000,20,0)', 10, 5, 3, 63, 83, '(t,1,C06019_ViCell_BLU_Reagent_Pak.bin,C06002_ViCell_FLR_Reagent_Pak.bin,C06001_ViCell_FLR_Reagent_Pak.bin)', false, true, 120, NULL, 1, 60, 30, true, 8, 7, 2147483648, 0, 120, '("",0,t,"","")', '("","",0,"",f)', '{"(1033,\"English (United States)\",en-US,t)","(1031,German,de-DE,f)","(3082,Spanish,es-ES,f)","(1036,French,fr-FR,f)","(1041,Japanese,ja-JP,f)","(4,\"Chinese( Simplified )\",zh-Hans,f)"}', '{"(0,2,110,t)","(1,3,40,t)","(2,4,160,t)","(3,5,70,t)","(4,6,70,t)","(5,7,70,t)","(6,8,100,t)","(7,9,60,t)","(8,10,170,t)","(9,11,70,t)","(10,12,130,t)","(11,13,160,t)"}', '(SampleSet,Sample,100,1,t,"\\\\Instrument\\\\Export",t,"\\\\Instrument\\\\Export",Summary,"\\\\Instrument\\\\Results",f,"\\\\Instrument\\\\CSV",0,1,5)', false, false, false, 0, false);


--
-- TOC entry 3276 (class 0 OID 142558)
-- Dependencies: 256
-- Data for Name: QcProcesses; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--



--
-- TOC entry 3278 (class 0 OID 142567)
-- Dependencies: 258
-- Data for Name: ReagentInfo; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--


--


INSERT INTO "ViCellInstrument"."CellHealthReagents" ("IdNum", "Type", "ID", "Name", "Volume", "Protected") VALUES (1, 1, '00000000-0000-0000-0000-000000000000', 'Trypan Blue', 0, true);
INSERT INTO "ViCellInstrument"."CellHealthReagents" ("IdNum", "Type", "ID", "Name", "Volume", "Protected") VALUES (2, 2, '00000000-0000-0000-0000-000000000000', 'Cleaning Agent', 0, true);
INSERT INTO "ViCellInstrument"."CellHealthReagents" ("IdNum", "Type", "ID", "Name", "Volume", "Protected") VALUES (3, 3, '00000000-0000-0000-0000-000000000000', 'Conditioning Solution', 0, true);
INSERT INTO "ViCellInstrument"."CellHealthReagents" ("IdNum", "Type", "ID", "Name", "Volume", "Protected") VALUES (4, 4, '00000000-0000-0000-0000-000000000000', 'Buffer Solution', 0, true);
INSERT INTO "ViCellInstrument"."CellHealthReagents" ("IdNum", "Type", "ID", "Name", "Volume", "Protected") VALUES (5, 5, '00000000-0000-0000-0000-000000000000', 'Diluent', 0, true);



--
-- TOC entry 3280 (class 0 OID 142576)
-- Dependencies: 260
-- Data for Name: Roles; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

INSERT INTO "ViCellInstrument"."Roles" ("RoleIdNum", "RoleID", "RoleName", "RoleType", "GroupMapList", "CellTypeIndexList", "InstrumentPermissions", "ApplicationPermissions", "Protected") VALUES (1, '1956600a-701f-4bf3-9f47-cfa74338cb7c', 'DefaultAdmin', 256, '{}', '{0,1,2,3,4,5,6}', -1, -1, true);
INSERT INTO "ViCellInstrument"."Roles" ("RoleIdNum", "RoleID", "RoleName", "RoleType", "GroupMapList", "CellTypeIndexList", "InstrumentPermissions", "ApplicationPermissions", "Protected") VALUES (2, 'd52c4971-0709-40fb-91f3-a505c46d29ee', 'DefaultElevated', 16, '{}', '{0,1,2,3,4}', -1, -1, true);
INSERT INTO "ViCellInstrument"."Roles" ("RoleIdNum", "RoleID", "RoleName", "RoleType", "GroupMapList", "CellTypeIndexList", "InstrumentPermissions", "ApplicationPermissions", "Protected") VALUES (3, '87bba3c9-00ea-4c95-ae33-f958af6a42db', 'DefaultUser', 1, '{}', '{0,1,2,3}', -1, -1, true);


--
-- TOC entry 3282 (class 0 OID 142585)
-- Dependencies: 262
-- Data for Name: SampleItems; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--



--
-- TOC entry 3284 (class 0 OID 142594)
-- Dependencies: 264
-- Data for Name: SampleSets; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--



--
-- TOC entry 3299 (class 0 OID 156906)
-- Dependencies: 279
-- Data for Name: Scheduler; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--



--
-- TOC entry 3286 (class 0 OID 142604)
-- Dependencies: 266
-- Data for Name: SignatureDefinitions; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

INSERT INTO "ViCellInstrument"."SignatureDefinitions" ("SignatureDefIdNum", "SignatureDefID", "ShortSignature", "ShortSignatureHash", "LongSignature", "LongSignatureHash", "Protected") VALUES (1, '1c1fc498-2556-4d82-ad2e-f17fec8738f1', 'REV', 'A6FDAB7318A0E469FDC9D3CCA3425F7A97C5200463A987B6284F95154E865D41', 'Reviewed', '085B3CE15D58FD371B34E8C740C68D2D3DFC193E24150FEA80E23FF87691F4B7', false);
INSERT INTO "ViCellInstrument"."SignatureDefinitions" ("SignatureDefIdNum", "SignatureDefID", "ShortSignature", "ShortSignatureHash", "LongSignature", "LongSignatureHash", "Protected") VALUES (2, '0a863686-c6b1-4780-bc18-5b23541c3dbf', 'APR', 'F426332AD546DFB70EC81C7CCFEA329F96C45CF49BCBD6E692B78E0203D52E8A', 'Approved', '17973463360605335781E00132F00B5683294786BA958E0FF5CC1AC54B764B3C', false);


--
-- TOC entry 3288 (class 0 OID 142613)
-- Dependencies: 268
-- Data for Name: SystemLogs; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--



--
-- TOC entry 3290 (class 0 OID 142622)
-- Dependencies: 270
-- Data for Name: UserProperties; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--



--
-- TOC entry 3292 (class 0 OID 142631)
-- Dependencies: 272
-- Data for Name: Users; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

INSERT INTO "ViCellInstrument"."Users" ("UserIdNum", "UserID", "Retired", "ADUser", "RoleID", "UserName", "DisplayName", "Comments", "UserEmail", "AuthenticatorList", "AuthenticatorDate", "LastLogin", "AttemptCount", "LanguageCode", "DefaultSampleName", "SaveNthIImage", "DisplayColumns", "DecimalPrecision", "ExportFolder", "DefaultResultFileName", "CSVFolder", "PdfExport", "AllowFastMode", "WashType", "Dilution", "DefaultCellTypeIndex", "NumUserCellTypes", "UserCellTypeIndexList", "UserAnalysisDefIndexList", "NumUserProperties", "UserPropertiesIndexList", "AppPermissions", "AppPermissionsHash", "InstrumentPermissions", "InstrumentPermissionsHash", "Protected") VALUES (1, '68cd72e5-1200-4f17-ab23-832b8884c69e', false, false, '1956600a-701f-4bf3-9f47-cfa74338cb7c', 'factory_admin', 'factory_admin', NULL, NULL, '{factory_admin:CD5D2C56C5A234EBBFFEC87E4FC4E2FFDD476ABB:1505EE967FBBC3AF6E5BC11ECA768482D11EE642DDDF2D4023BD6FE656F39B08:262889640FA4BCBC318F98C27A82189E9C5AB6AE40EB8C8CF1AA15232EF747CE:73F0867D3286442A784E2B8582557520D2114D1499C368764543172933224D9A}', '1970-01-01 00:00:00', '1970-01-01 00:00:00', 0, NULL, 'Sample', 1, '{"(0,2,110,t)","(1,3,40,t)","(2,4,160,t)","(3,5,70,t)","(4,6,70,t)","(5,7,70,t)","(6,8,100,t)","(7,9,60,t)","(8,10,170,t)","(9,11,70,t)","(10,12,130,t)","(11,13,160,t)"}', 5, '\\Instrument\\Export\\factory_admin', 'Summary', '\\Instrument\\Export\\CSV\\factory_admin', true, true, 0, 1, 0, 7, '{0,1,2,3,4,5,6}', '{0,0,0,0,0,0,0}', 0, '{}', -1, '', -1, '', true);


--
-- TOC entry 3294 (class 0 OID 142644)
-- Dependencies: 274
-- Data for Name: Workflows; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--



--
-- TOC entry 3296 (class 0 OID 142653)
-- Dependencies: 276
-- Data for Name: Worklists; Type: TABLE DATA; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--



--
-- TOC entry 3406 (class 0 OID 0)
-- Dependencies: 219
-- Name: Analyses_AnalysisIdNum_seq; Type: SEQUENCE SET; Schema: ViCellData; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellData"."Analyses_AnalysisIdNum_seq"', 1, false);


--
-- TOC entry 3407 (class 0 OID 0)
-- Dependencies: 221
-- Name: DetailedResults_DetailedResultIdNum_seq; Type: SEQUENCE SET; Schema: ViCellData; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellData"."DetailedResults_DetailedResultIdNum_seq"', 1, false);


--
-- TOC entry 3408 (class 0 OID 0)
-- Dependencies: 223
-- Name: ImageReferences_ImageIdNum_seq; Type: SEQUENCE SET; Schema: ViCellData; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellData"."ImageReferences_ImageIdNum_seq"', 1, false);


--
-- TOC entry 3409 (class 0 OID 0)
-- Dependencies: 225
-- Name: ImageResults_ResultIdNum_seq; Type: SEQUENCE SET; Schema: ViCellData; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellData"."ImageResults_ResultIdNum_seq"', 1, false);


--
-- TOC entry 3410 (class 0 OID 0)
-- Dependencies: 227
-- Name: ImageSequences_ImageSequenceIdNum_seq; Type: SEQUENCE SET; Schema: ViCellData; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellData"."ImageSequences_ImageSequenceIdNum_seq"', 1, false);


--
-- TOC entry 3411 (class 0 OID 0)
-- Dependencies: 229
-- Name: ImageSets_ImageSetIdNum_seq; Type: SEQUENCE SET; Schema: ViCellData; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellData"."ImageSets_ImageSetIdNum_seq"', 1, false);


--
-- TOC entry 3412 (class 0 OID 0)
-- Dependencies: 231
-- Name: SResults_SResultIdNum_seq; Type: SEQUENCE SET; Schema: ViCellData; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellData"."SResults_SResultIdNum_seq"', 1, false);


--
-- TOC entry 3413 (class 0 OID 0)
-- Dependencies: 233
-- Name: SampleProperties_SampleIdNum_seq; Type: SEQUENCE SET; Schema: ViCellData; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellData"."SampleProperties_SampleIdNum_seq"', 1, false);


--
-- TOC entry 3414 (class 0 OID 0)
-- Dependencies: 235
-- Name: SummaryResults_SummaryResultIdNum_seq; Type: SEQUENCE SET; Schema: ViCellData; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellData"."SummaryResults_SummaryResultIdNum_seq"', 1, false);


--
-- TOC entry 3415 (class 0 OID 0)
-- Dependencies: 237
-- Name: AnalysisDefinitions_AnalysisDefinitionIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."AnalysisDefinitions_AnalysisDefinitionIdNum_seq"', 8, true);


--
-- TOC entry 3416 (class 0 OID 0)
-- Dependencies: 239
-- Name: AnalysisInputSettings_SettingsIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."AnalysisInputSettings_SettingsIdNum_seq"', 2, true);


--
-- TOC entry 3417 (class 0 OID 0)
-- Dependencies: 241
-- Name: AnalysisParams_AnalysisParamIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."AnalysisParams_AnalysisParamIdNum_seq"', 16, true);


--
-- TOC entry 3418 (class 0 OID 0)
-- Dependencies: 243
-- Name: BioProcesses_BioProcessIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."BioProcesses_BioProcessIdNum_seq"', 1, false);


--
-- TOC entry 3419 (class 0 OID 0)
-- Dependencies: 245
-- Name: Calibrations_CalibrationIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."Calibrations_CalibrationIdNum_seq"', 2, true);


--
-- TOC entry 3420 (class 0 OID 0)
-- Dependencies: 247
-- Name: CellTypes_CellTypeIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."CellTypes_CellTypeIdNum_seq"', 7, true);


--
-- TOC entry 3421 (class 0 OID 0)
-- Dependencies: 249
-- Name: IlluminatorTypes_IlluminatorIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."IlluminatorTypes_IlluminatorIdNum_seq"', 1, true);


--
-- TOC entry 3422 (class 0 OID 0)
-- Dependencies: 251
-- Name: ImageAnalysisCellIdentParams_IdentParamIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."ImageAnalysisCellIdentParams_IdentParamIdNum_seq"', 1, false);


--
-- TOC entry 3423 (class 0 OID 0)
-- Dependencies: 253
-- Name: ImageAnalysisParams_ImageAnalysisParamIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."ImageAnalysisParams_ImageAnalysisParamIdNum_seq"', 1, true);


--
-- TOC entry 3424 (class 0 OID 0)
-- Dependencies: 255
-- Name: InstrumentConfig_InstrumentIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."InstrumentConfig_InstrumentIdNum_seq"', 1, true);


--
-- TOC entry 3425 (class 0 OID 0)
-- Dependencies: 257
-- Name: QcProcesses_QcIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."QcProcesses_QcIdNum_seq"', 1, false);


--
-- TOC entry 3426 (class 0 OID 0)
-- Dependencies: 259
-- Name: ReagentInfo_ReagentIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."ReagentInfo_ReagentIdNum_seq"', 1, false);



SELECT pg_catalog.setval('"ViCellInstrument"."CellHealthReagents_IdNum_seq"', 1, false);


--
-- TOC entry 3427 (class 0 OID 0)
-- Dependencies: 261
-- Name: Roles_RoleIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."Roles_RoleIdNum_seq"', 3, true);


--
-- TOC entry 3428 (class 0 OID 0)
-- Dependencies: 263
-- Name: SampleItems_SampleItemIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."SampleItems_SampleItemIdNum_seq"', 1, false);


--
-- TOC entry 3429 (class 0 OID 0)
-- Dependencies: 265
-- Name: SampleSets_SampleSetIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."SampleSets_SampleSetIdNum_seq"', 1, false);


--
-- TOC entry 3430 (class 0 OID 0)
-- Dependencies: 278
-- Name: Scheduler_SchedulerConfigIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."Scheduler_SchedulerConfigIdNum_seq"', 1, false);


--
-- TOC entry 3431 (class 0 OID 0)
-- Dependencies: 267
-- Name: SignatureDefinitions_SignatureDefIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."SignatureDefinitions_SignatureDefIdNum_seq"', 2, true);


--
-- TOC entry 3432 (class 0 OID 0)
-- Dependencies: 269
-- Name: SystemLogs_EntryIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."SystemLogs_EntryIdNum_seq"', 1, false);


--
-- TOC entry 3433 (class 0 OID 0)
-- Dependencies: 271
-- Name: UserProperties_PropertyIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."UserProperties_PropertyIdNum_seq"', 1, false);


--
-- TOC entry 3434 (class 0 OID 0)
-- Dependencies: 273
-- Name: Users_UserIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."Users_UserIdNum_seq"', 1, true);


--
-- TOC entry 3435 (class 0 OID 0)
-- Dependencies: 275
-- Name: Workflows_WorkflowIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."Workflows_WorkflowIdNum_seq"', 1, false);


--
-- TOC entry 3436 (class 0 OID 0)
-- Dependencies: 277
-- Name: Worklists_WorklistIdNum_seq; Type: SEQUENCE SET; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

SELECT pg_catalog.setval('"ViCellInstrument"."Worklists_WorklistIdNum_seq"', 1, false);


--
-- TOC entry 3056 (class 2606 OID 142694)
-- Name: Analyses Analyses_pkey; Type: CONSTRAINT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."Analyses"
    ADD CONSTRAINT "Analyses_pkey" PRIMARY KEY ("AnalysisID");


--
-- TOC entry 3058 (class 2606 OID 142696)
-- Name: DetailedResults DetailedResults_pkey; Type: CONSTRAINT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."DetailedResults"
    ADD CONSTRAINT "DetailedResults_pkey" PRIMARY KEY ("DetailedResultID");


--
-- TOC entry 3060 (class 2606 OID 142698)
-- Name: ImageReferences ImageReferences_pkey; Type: CONSTRAINT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."ImageReferences"
    ADD CONSTRAINT "ImageReferences_pkey" PRIMARY KEY ("ImageID");


--
-- TOC entry 3062 (class 2606 OID 142700)
-- Name: ImageResults ImageResultsMap_pkey; Type: CONSTRAINT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."ImageResults"
    ADD CONSTRAINT "ImageResultsMap_pkey" PRIMARY KEY ("ResultID");


--
-- TOC entry 3064 (class 2606 OID 142702)
-- Name: ImageSequences ImageSequences_pkey; Type: CONSTRAINT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."ImageSequences"
    ADD CONSTRAINT "ImageSequences_pkey" PRIMARY KEY ("ImageSequenceID");


--
-- TOC entry 3066 (class 2606 OID 142704)
-- Name: ImageSets ImageSets_pkey; Type: CONSTRAINT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."ImageSets"
    ADD CONSTRAINT "ImageSets_pkey" PRIMARY KEY ("ImageSetID");


--
-- TOC entry 3068 (class 2606 OID 142706)
-- Name: SResults SResults_pkey; Type: CONSTRAINT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."SResults"
    ADD CONSTRAINT "SResults_pkey" PRIMARY KEY ("SResultID");


--
-- TOC entry 3070 (class 2606 OID 142708)
-- Name: SampleProperties SampleProperties_pkey; Type: CONSTRAINT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."SampleProperties"
    ADD CONSTRAINT "SampleProperties_pkey" PRIMARY KEY ("SampleID");


--
-- TOC entry 3072 (class 2606 OID 142710)
-- Name: SummaryResults SummaryResults_pkey; Type: CONSTRAINT; Schema: ViCellData; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellData"."SummaryResults"
    ADD CONSTRAINT "SummaryResults_pkey" PRIMARY KEY ("SummaryResultID");


--
-- TOC entry 3074 (class 2606 OID 142712)
-- Name: AnalysisDefinitions AnalysisDefinitions_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."AnalysisDefinitions"
    ADD CONSTRAINT "AnalysisDefinitions_pkey" PRIMARY KEY ("AnalysisDefinitionID");


--
-- TOC entry 3076 (class 2606 OID 142714)
-- Name: AnalysisInputSettings AnalysisInputSettings_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."AnalysisInputSettings"
    ADD CONSTRAINT "AnalysisInputSettings_pkey" PRIMARY KEY ("SettingsID");


--
-- TOC entry 3078 (class 2606 OID 142716)
-- Name: AnalysisParams AnalysisParams_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."AnalysisParams"
    ADD CONSTRAINT "AnalysisParams_pkey" PRIMARY KEY ("AnalysisParamID");


--
-- TOC entry 3080 (class 2606 OID 142718)
-- Name: BioProcesses BioProcesses_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."BioProcesses"
    ADD CONSTRAINT "BioProcesses_pkey" PRIMARY KEY ("BioProcessID");


--
-- TOC entry 3082 (class 2606 OID 142720)
-- Name: Calibrations Calibrations_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."Calibrations"
    ADD CONSTRAINT "Calibrations_pkey" PRIMARY KEY ("CalibrationID");


--
-- TOC entry 3084 (class 2606 OID 142722)
-- Name: CellTypes CellTypes_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."CellTypes"
    ADD CONSTRAINT "CellTypes_pkey" PRIMARY KEY ("CellTypeID");


--
-- TOC entry 3086 (class 2606 OID 142724)
-- Name: IlluminatorTypes IlluminatorTypes_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."IlluminatorTypes"
    ADD CONSTRAINT "IlluminatorTypes_pkey" PRIMARY KEY ("IlluminatorIndex");


--
-- TOC entry 3088 (class 2606 OID 142726)
-- Name: ImageAnalysisCellIdentParams ImageAnalysisCellIdentParams_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."ImageAnalysisCellIdentParams"
    ADD CONSTRAINT "ImageAnalysisCellIdentParams_pkey" PRIMARY KEY ("IdentParamID");


--
-- TOC entry 3090 (class 2606 OID 142728)
-- Name: ImageAnalysisParams ImageAnalysisParams_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."ImageAnalysisParams"
    ADD CONSTRAINT "ImageAnalysisParams_pkey" PRIMARY KEY ("ImageAnalysisParamID");


--
-- TOC entry 3092 (class 2606 OID 142730)
-- Name: InstrumentConfig InstrumentConfig_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."InstrumentConfig"
    ADD CONSTRAINT "InstrumentConfig_pkey" PRIMARY KEY ("InstrumentIdNum");


--
-- TOC entry 3110 (class 2606 OID 142732)
-- Name: Users InstrumentUsers_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."Users"
    ADD CONSTRAINT "InstrumentUsers_pkey" PRIMARY KEY ("UserID");


--
-- TOC entry 3094 (class 2606 OID 142734)
-- Name: QcProcesses QcProcesses_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."QcProcesses"
    ADD CONSTRAINT "QcProcesses_pkey" PRIMARY KEY ("QcID");


--
-- TOC entry 3096 (class 2606 OID 142736)
-- Name: ReagentInfo ReagentInfo_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."ReagentInfo"
    ADD CONSTRAINT "ReagentInfo_pkey" PRIMARY KEY ("ReagentIdNum");


--


ALTER TABLE ONLY "ViCellInstrument"."CellHealthReagents"
    ADD CONSTRAINT "CellHealthReagents_pkey" PRIMARY KEY ("IdNum");


--
-- TOC entry 3098 (class 2606 OID 142738)
-- Name: Roles Roles_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."Roles"
    ADD CONSTRAINT "Roles_pkey" PRIMARY KEY ("RoleID");


--
-- TOC entry 3100 (class 2606 OID 142740)
-- Name: SampleItems SampleItems_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."SampleItems"
    ADD CONSTRAINT "SampleItems_pkey" PRIMARY KEY ("SampleItemIdNum");


--
-- TOC entry 3102 (class 2606 OID 142742)
-- Name: SampleSets SampleSets_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."SampleSets"
    ADD CONSTRAINT "SampleSets_pkey" PRIMARY KEY ("SampleSetIdNum");


--
-- TOC entry 3116 (class 2606 OID 156914)
-- Name: Scheduler Scheduler_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."Scheduler"
    ADD CONSTRAINT "Scheduler_pkey" PRIMARY KEY ("SchedulerConfigID");


--
-- TOC entry 3104 (class 2606 OID 142744)
-- Name: SignatureDefinitions SignatureDefinitions_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."SignatureDefinitions"
    ADD CONSTRAINT "SignatureDefinitions_pkey" PRIMARY KEY ("SignatureDefID");


--
-- TOC entry 3106 (class 2606 OID 142746)
-- Name: SystemLogs SystemLogs_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."SystemLogs"
    ADD CONSTRAINT "SystemLogs_pkey" PRIMARY KEY ("EntryIdNum");


--
-- TOC entry 3108 (class 2606 OID 142748)
-- Name: UserProperties UserProperties_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."UserProperties"
    ADD CONSTRAINT "UserProperties_pkey" PRIMARY KEY ("PropertyIndex");


--
-- TOC entry 3112 (class 2606 OID 142750)
-- Name: Workflows Workflows_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."Workflows"
    ADD CONSTRAINT "Workflows_pkey" PRIMARY KEY ("WorkflowID");


--
-- TOC entry 3114 (class 2606 OID 142752)
-- Name: Worklists Worklists_pkey; Type: CONSTRAINT; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

ALTER TABLE ONLY "ViCellInstrument"."Worklists"
    ADD CONSTRAINT "Worklists_pkey" PRIMARY KEY ("WorklistIdNum");


--
-- TOC entry 3306 (class 0 OID 0)
-- Dependencies: 9
-- Name: SCHEMA "ViCellData"; Type: ACL; Schema: -; Owner: BCIViCellAdmin
--

REVOKE ALL ON SCHEMA "ViCellData" FROM "BCIViCellAdmin";
GRANT ALL ON SCHEMA "ViCellData" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SCHEMA "ViCellData" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SCHEMA "ViCellData" TO "ViCellAdmin";
GRANT ALL ON SCHEMA "ViCellData" TO "ViCellDBAdmin";
GRANT ALL ON SCHEMA "ViCellData" TO "ViCellInstrumentUser";
GRANT USAGE ON SCHEMA "ViCellData" TO "DbBackupUser";


--
-- TOC entry 3307 (class 0 OID 0)
-- Dependencies: 4
-- Name: SCHEMA "ViCellInstrument"; Type: ACL; Schema: -; Owner: BCIViCellAdmin
--

REVOKE ALL ON SCHEMA "ViCellInstrument" FROM "BCIViCellAdmin";
GRANT ALL ON SCHEMA "ViCellInstrument" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SCHEMA "ViCellInstrument" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SCHEMA "ViCellInstrument" TO "ViCellAdmin";
GRANT ALL ON SCHEMA "ViCellInstrument" TO "ViCellDBAdmin";
GRANT ALL ON SCHEMA "ViCellInstrument" TO "ViCellInstrumentUser";
GRANT USAGE ON SCHEMA "ViCellInstrument" TO "DbBackupUser";


--
-- TOC entry 3309 (class 0 OID 0)
-- Dependencies: 7
-- Name: SCHEMA public; Type: ACL; Schema: -; Owner: postgres
--

GRANT ALL ON SCHEMA public TO PUBLIC;


--
-- TOC entry 3311 (class 0 OID 0)
-- Dependencies: 218
-- Name: TABLE "Analyses"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellData"."Analyses" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."Analyses" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."Analyses" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."Analyses" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."Analyses" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellData"."Analyses" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellData"."Analyses" TO "DbBackupUser";


--
-- TOC entry 3313 (class 0 OID 0)
-- Dependencies: 219
-- Name: SEQUENCE "Analyses_AnalysisIdNum_seq"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellData"."Analyses_AnalysisIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."Analyses_AnalysisIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."Analyses_AnalysisIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."Analyses_AnalysisIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."Analyses_AnalysisIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."Analyses_AnalysisIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellData"."Analyses_AnalysisIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3314 (class 0 OID 0)
-- Dependencies: 220
-- Name: TABLE "DetailedResults"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellData"."DetailedResults" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."DetailedResults" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."DetailedResults" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."DetailedResults" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."DetailedResults" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellData"."DetailedResults" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellData"."DetailedResults" TO "DbBackupUser";


--
-- TOC entry 3316 (class 0 OID 0)
-- Dependencies: 221
-- Name: SEQUENCE "DetailedResults_DetailedResultIdNum_seq"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellData"."DetailedResults_DetailedResultIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."DetailedResults_DetailedResultIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."DetailedResults_DetailedResultIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."DetailedResults_DetailedResultIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."DetailedResults_DetailedResultIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."DetailedResults_DetailedResultIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellData"."DetailedResults_DetailedResultIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3318 (class 0 OID 0)
-- Dependencies: 222
-- Name: TABLE "ImageReferences"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellData"."ImageReferences" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."ImageReferences" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."ImageReferences" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."ImageReferences" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."ImageReferences" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellData"."ImageReferences" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellData"."ImageReferences" TO "DbBackupUser";


--
-- TOC entry 3320 (class 0 OID 0)
-- Dependencies: 223
-- Name: SEQUENCE "ImageReferences_ImageIdNum_seq"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellData"."ImageReferences_ImageIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."ImageReferences_ImageIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."ImageReferences_ImageIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."ImageReferences_ImageIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."ImageReferences_ImageIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."ImageReferences_ImageIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellData"."ImageReferences_ImageIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3321 (class 0 OID 0)
-- Dependencies: 224
-- Name: TABLE "ImageResults"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellData"."ImageResults" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."ImageResults" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."ImageResults" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."ImageResults" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."ImageResults" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellData"."ImageResults" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellData"."ImageResults" TO "DbBackupUser";


--
-- TOC entry 3323 (class 0 OID 0)
-- Dependencies: 225
-- Name: SEQUENCE "ImageResults_ResultIdNum_seq"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellData"."ImageResults_ResultIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."ImageResults_ResultIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."ImageResults_ResultIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."ImageResults_ResultIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."ImageResults_ResultIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."ImageResults_ResultIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellData"."ImageResults_ResultIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3325 (class 0 OID 0)
-- Dependencies: 226
-- Name: TABLE "ImageSequences"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellData"."ImageSequences" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."ImageSequences" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."ImageSequences" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."ImageSequences" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."ImageSequences" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellData"."ImageSequences" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellData"."ImageSequences" TO "DbBackupUser";


--
-- TOC entry 3327 (class 0 OID 0)
-- Dependencies: 227
-- Name: SEQUENCE "ImageSequences_ImageSequenceIdNum_seq"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellData"."ImageSequences_ImageSequenceIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."ImageSequences_ImageSequenceIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."ImageSequences_ImageSequenceIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."ImageSequences_ImageSequenceIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."ImageSequences_ImageSequenceIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."ImageSequences_ImageSequenceIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellData"."ImageSequences_ImageSequenceIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3329 (class 0 OID 0)
-- Dependencies: 228
-- Name: TABLE "ImageSets"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellData"."ImageSets" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."ImageSets" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."ImageSets" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."ImageSets" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."ImageSets" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellData"."ImageSets" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellData"."ImageSets" TO "DbBackupUser";


--
-- TOC entry 3331 (class 0 OID 0)
-- Dependencies: 229
-- Name: SEQUENCE "ImageSets_ImageSetIdNum_seq"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellData"."ImageSets_ImageSetIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."ImageSets_ImageSetIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."ImageSets_ImageSetIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."ImageSets_ImageSetIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."ImageSets_ImageSetIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."ImageSets_ImageSetIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellData"."ImageSets_ImageSetIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3332 (class 0 OID 0)
-- Dependencies: 230
-- Name: TABLE "SResults"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellData"."SResults" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."SResults" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."SResults" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."SResults" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."SResults" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,UPDATE ON TABLE "ViCellData"."SResults" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellData"."SResults" TO "DbBackupUser";


--
-- TOC entry 3334 (class 0 OID 0)
-- Dependencies: 231
-- Name: SEQUENCE "SResults_SResultIdNum_seq"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellData"."SResults_SResultIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."SResults_SResultIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."SResults_SResultIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."SResults_SResultIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."SResults_SResultIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."SResults_SResultIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellData"."SResults_SResultIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3335 (class 0 OID 0)
-- Dependencies: 232
-- Name: TABLE "SampleProperties"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellData"."SampleProperties" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."SampleProperties" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."SampleProperties" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."SampleProperties" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."SampleProperties" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellData"."SampleProperties" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellData"."SampleProperties" TO "DbBackupUser";


--
-- TOC entry 3337 (class 0 OID 0)
-- Dependencies: 233
-- Name: SEQUENCE "SampleProperties_SampleIdNum_seq"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellData"."SampleProperties_SampleIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."SampleProperties_SampleIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."SampleProperties_SampleIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."SampleProperties_SampleIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."SampleProperties_SampleIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."SampleProperties_SampleIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellData"."SampleProperties_SampleIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3338 (class 0 OID 0)
-- Dependencies: 234
-- Name: TABLE "SummaryResults"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellData"."SummaryResults" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."SummaryResults" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."SummaryResults" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellData"."SummaryResults" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellData"."SummaryResults" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellData"."SummaryResults" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellData"."SummaryResults" TO "DbBackupUser";


--
-- TOC entry 3340 (class 0 OID 0)
-- Dependencies: 235
-- Name: SEQUENCE "SummaryResults_SummaryResultIdNum_seq"; Type: ACL; Schema: ViCellData; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellData"."SummaryResults_SummaryResultIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."SummaryResults_SummaryResultIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."SummaryResults_SummaryResultIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellData"."SummaryResults_SummaryResultIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."SummaryResults_SummaryResultIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellData"."SummaryResults_SummaryResultIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellData"."SummaryResults_SummaryResultIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3341 (class 0 OID 0)
-- Dependencies: 236
-- Name: TABLE "AnalysisDefinitions"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."AnalysisDefinitions" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."AnalysisDefinitions" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."AnalysisDefinitions" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."AnalysisDefinitions" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."AnalysisDefinitions" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."AnalysisDefinitions" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."AnalysisDefinitions" TO "DbBackupUser";


--
-- TOC entry 3343 (class 0 OID 0)
-- Dependencies: 237
-- Name: SEQUENCE "AnalysisDefinitions_AnalysisDefinitionIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."AnalysisDefinitions_AnalysisDefinitionIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."AnalysisDefinitions_AnalysisDefinitionIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."AnalysisDefinitions_AnalysisDefinitionIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."AnalysisDefinitions_AnalysisDefinitionIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."AnalysisDefinitions_AnalysisDefinitionIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."AnalysisDefinitions_AnalysisDefinitionIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."AnalysisDefinitions_AnalysisDefinitionIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3344 (class 0 OID 0)
-- Dependencies: 238
-- Name: TABLE "AnalysisInputSettings"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."AnalysisInputSettings" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."AnalysisInputSettings" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."AnalysisInputSettings" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."AnalysisInputSettings" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."AnalysisInputSettings" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,UPDATE ON TABLE "ViCellInstrument"."AnalysisInputSettings" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."AnalysisInputSettings" TO "DbBackupUser";


--
-- TOC entry 3346 (class 0 OID 0)
-- Dependencies: 239
-- Name: SEQUENCE "AnalysisInputSettings_SettingsIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."AnalysisInputSettings_SettingsIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."AnalysisInputSettings_SettingsIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."AnalysisInputSettings_SettingsIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."AnalysisInputSettings_SettingsIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."AnalysisInputSettings_SettingsIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."AnalysisInputSettings_SettingsIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."AnalysisInputSettings_SettingsIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3347 (class 0 OID 0)
-- Dependencies: 240
-- Name: TABLE "AnalysisParams"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."AnalysisParams" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."AnalysisParams" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."AnalysisParams" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."AnalysisParams" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."AnalysisParams" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."AnalysisParams" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."AnalysisParams" TO "DbBackupUser";


--
-- TOC entry 3349 (class 0 OID 0)
-- Dependencies: 241
-- Name: SEQUENCE "AnalysisParams_AnalysisParamIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."AnalysisParams_AnalysisParamIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."AnalysisParams_AnalysisParamIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."AnalysisParams_AnalysisParamIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."AnalysisParams_AnalysisParamIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."AnalysisParams_AnalysisParamIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."AnalysisParams_AnalysisParamIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."AnalysisParams_AnalysisParamIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3350 (class 0 OID 0)
-- Dependencies: 242
-- Name: TABLE "BioProcesses"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."BioProcesses" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."BioProcesses" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."BioProcesses" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."BioProcesses" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."BioProcesses" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."BioProcesses" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."BioProcesses" TO "DbBackupUser";


--
-- TOC entry 3352 (class 0 OID 0)
-- Dependencies: 243
-- Name: SEQUENCE "BioProcesses_BioProcessIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."BioProcesses_BioProcessIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."BioProcesses_BioProcessIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."BioProcesses_BioProcessIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."BioProcesses_BioProcessIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."BioProcesses_BioProcessIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."BioProcesses_BioProcessIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."BioProcesses_BioProcessIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3353 (class 0 OID 0)
-- Dependencies: 244
-- Name: TABLE "Calibrations"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."Calibrations" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."Calibrations" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."Calibrations" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."Calibrations" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."Calibrations" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,UPDATE ON TABLE "ViCellInstrument"."Calibrations" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."Calibrations" TO "DbBackupUser";


--
-- TOC entry 3355 (class 0 OID 0)
-- Dependencies: 245
-- Name: SEQUENCE "Calibrations_CalibrationIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."Calibrations_CalibrationIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Calibrations_CalibrationIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."Calibrations_CalibrationIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."Calibrations_CalibrationIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Calibrations_CalibrationIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Calibrations_CalibrationIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."Calibrations_CalibrationIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3356 (class 0 OID 0)
-- Dependencies: 246
-- Name: TABLE "CellTypes"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."CellTypes" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."CellTypes" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."CellTypes" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."CellTypes" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."CellTypes" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."CellTypes" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."CellTypes" TO "DbBackupUser";


--
-- TOC entry 3358 (class 0 OID 0)
-- Dependencies: 247
-- Name: SEQUENCE "CellTypes_CellTypeIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."CellTypes_CellTypeIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."CellTypes_CellTypeIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."CellTypes_CellTypeIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."CellTypes_CellTypeIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."CellTypes_CellTypeIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."CellTypes_CellTypeIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."CellTypes_CellTypeIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3359 (class 0 OID 0)
-- Dependencies: 248
-- Name: TABLE "IlluminatorTypes"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."IlluminatorTypes" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."IlluminatorTypes" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."IlluminatorTypes" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."IlluminatorTypes" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."IlluminatorTypes" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."IlluminatorTypes" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."IlluminatorTypes" TO "DbBackupUser";


--
-- TOC entry 3361 (class 0 OID 0)
-- Dependencies: 249
-- Name: SEQUENCE "IlluminatorTypes_IlluminatorIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."IlluminatorTypes_IlluminatorIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."IlluminatorTypes_IlluminatorIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."IlluminatorTypes_IlluminatorIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."IlluminatorTypes_IlluminatorIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."IlluminatorTypes_IlluminatorIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."IlluminatorTypes_IlluminatorIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."IlluminatorTypes_IlluminatorIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3362 (class 0 OID 0)
-- Dependencies: 250
-- Name: TABLE "ImageAnalysisCellIdentParams"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."ImageAnalysisCellIdentParams" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."ImageAnalysisCellIdentParams" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."ImageAnalysisCellIdentParams" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."ImageAnalysisCellIdentParams" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."ImageAnalysisCellIdentParams" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."ImageAnalysisCellIdentParams" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."ImageAnalysisCellIdentParams" TO "DbBackupUser";


--
-- TOC entry 3364 (class 0 OID 0)
-- Dependencies: 251
-- Name: SEQUENCE "ImageAnalysisCellIdentParams_IdentParamIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."ImageAnalysisCellIdentParams_IdentParamIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."ImageAnalysisCellIdentParams_IdentParamIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."ImageAnalysisCellIdentParams_IdentParamIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."ImageAnalysisCellIdentParams_IdentParamIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."ImageAnalysisCellIdentParams_IdentParamIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."ImageAnalysisCellIdentParams_IdentParamIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."ImageAnalysisCellIdentParams_IdentParamIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3365 (class 0 OID 0)
-- Dependencies: 252
-- Name: TABLE "ImageAnalysisParams"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."ImageAnalysisParams" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."ImageAnalysisParams" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."ImageAnalysisParams" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."ImageAnalysisParams" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."ImageAnalysisParams" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."ImageAnalysisParams" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."ImageAnalysisParams" TO "DbBackupUser";


--
-- TOC entry 3367 (class 0 OID 0)
-- Dependencies: 253
-- Name: SEQUENCE "ImageAnalysisParams_ImageAnalysisParamIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."ImageAnalysisParams_ImageAnalysisParamIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."ImageAnalysisParams_ImageAnalysisParamIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."ImageAnalysisParams_ImageAnalysisParamIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."ImageAnalysisParams_ImageAnalysisParamIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."ImageAnalysisParams_ImageAnalysisParamIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."ImageAnalysisParams_ImageAnalysisParamIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."ImageAnalysisParams_ImageAnalysisParamIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3368 (class 0 OID 0)
-- Dependencies: 254
-- Name: TABLE "InstrumentConfig"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."InstrumentConfig" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."InstrumentConfig" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."InstrumentConfig" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."InstrumentConfig" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."InstrumentConfig" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."InstrumentConfig" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."InstrumentConfig" TO "DbBackupUser";


--
-- TOC entry 3370 (class 0 OID 0)
-- Dependencies: 255
-- Name: SEQUENCE "InstrumentConfig_InstrumentIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."InstrumentConfig_InstrumentIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."InstrumentConfig_InstrumentIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."InstrumentConfig_InstrumentIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."InstrumentConfig_InstrumentIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."InstrumentConfig_InstrumentIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."InstrumentConfig_InstrumentIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."InstrumentConfig_InstrumentIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3371 (class 0 OID 0)
-- Dependencies: 256
-- Name: TABLE "QcProcesses"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."QcProcesses" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."QcProcesses" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."QcProcesses" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."QcProcesses" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."QcProcesses" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."QcProcesses" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."QcProcesses" TO "DbBackupUser";


--
-- TOC entry 3373 (class 0 OID 0)
-- Dependencies: 257
-- Name: SEQUENCE "QcProcesses_QcIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."QcProcesses_QcIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."QcProcesses_QcIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."QcProcesses_QcIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."QcProcesses_QcIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."QcProcesses_QcIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."QcProcesses_QcIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."QcProcesses_QcIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3374 (class 0 OID 0)
-- Dependencies: 258
-- Name: TABLE "ReagentInfo"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."ReagentInfo" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."ReagentInfo" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."ReagentInfo" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."ReagentInfo" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."ReagentInfo" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."ReagentInfo" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."ReagentInfo" TO "DbBackupUser";


--
-- TOC entry 3376 (class 0 OID 0)
-- Dependencies: 259
-- Name: SEQUENCE "ReagentInfo_ReagentIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."ReagentInfo_ReagentIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."ReagentInfo_ReagentIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."ReagentInfo_ReagentIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."ReagentInfo_ReagentIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."ReagentInfo_ReagentIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."ReagentInfo_ReagentIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."ReagentInfo_ReagentIdNum_seq" TO "DbBackupUser";



REVOKE ALL ON TABLE "ViCellInstrument"."CellHealthReagents" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."CellHealthReagents" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."CellHealthReagents" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."CellHealthReagents" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."CellHealthReagents" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."CellHealthReagents" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."CellHealthReagents" TO "DbBackupUser";


REVOKE ALL ON SEQUENCE "ViCellInstrument"."CellHealthReagents_IdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."CellHealthReagents_IdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."CellHealthReagents_IdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."CellHealthReagents_IdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."CellHealthReagents_IdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."CellHealthReagents_IdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."CellHealthReagents_IdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3377 (class 0 OID 0)
-- Dependencies: 260
-- Name: TABLE "Roles"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."Roles" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."Roles" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."Roles" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."Roles" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."Roles" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."Roles" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."Roles" TO "DbBackupUser";


--
-- TOC entry 3379 (class 0 OID 0)
-- Dependencies: 261
-- Name: SEQUENCE "Roles_RoleIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."Roles_RoleIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Roles_RoleIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."Roles_RoleIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."Roles_RoleIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Roles_RoleIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Roles_RoleIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."Roles_RoleIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3380 (class 0 OID 0)
-- Dependencies: 262
-- Name: TABLE "SampleItems"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."SampleItems" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."SampleItems" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."SampleItems" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."SampleItems" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."SampleItems" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."SampleItems" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."SampleItems" TO "DbBackupUser";


--
-- TOC entry 3382 (class 0 OID 0)
-- Dependencies: 263
-- Name: SEQUENCE "SampleItems_SampleItemIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."SampleItems_SampleItemIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."SampleItems_SampleItemIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."SampleItems_SampleItemIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."SampleItems_SampleItemIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."SampleItems_SampleItemIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."SampleItems_SampleItemIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."SampleItems_SampleItemIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3383 (class 0 OID 0)
-- Dependencies: 264
-- Name: TABLE "SampleSets"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."SampleSets" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."SampleSets" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."SampleSets" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."SampleSets" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."SampleSets" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."SampleSets" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."SampleSets" TO "DbBackupUser";


--
-- TOC entry 3385 (class 0 OID 0)
-- Dependencies: 265
-- Name: SEQUENCE "SampleSets_SampleSetIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."SampleSets_SampleSetIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."SampleSets_SampleSetIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."SampleSets_SampleSetIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."SampleSets_SampleSetIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."SampleSets_SampleSetIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."SampleSets_SampleSetIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."SampleSets_SampleSetIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3386 (class 0 OID 0)
-- Dependencies: 279
-- Name: TABLE "Scheduler"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."Scheduler" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."Scheduler" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."Scheduler" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."Scheduler" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."Scheduler" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."Scheduler" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."Scheduler" TO "DbBackupUser";


--
-- TOC entry 3385 (class 0 OID 0)
-- Dependencies: 265
-- Name: SEQUENCE "Scheduler_SchedulerConfigIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."Scheduler_SchedulerConfigIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Scheduler_SchedulerConfigIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."Scheduler_SchedulerConfigIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."Scheduler_SchedulerConfigIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Scheduler_SchedulerConfigIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Scheduler_SchedulerConfigIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."Scheduler_SchedulerConfigIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3388 (class 0 OID 0)
-- Dependencies: 266
-- Name: TABLE "SignatureDefinitions"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."SignatureDefinitions" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."SignatureDefinitions" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."SignatureDefinitions" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."SignatureDefinitions" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."SignatureDefinitions" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."SignatureDefinitions" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."SignatureDefinitions" TO "DbBackupUser";


--
-- TOC entry 3390 (class 0 OID 0)
-- Dependencies: 267
-- Name: SEQUENCE "SignatureDefinitions_SignatureDefIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."SignatureDefinitions_SignatureDefIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."SignatureDefinitions_SignatureDefIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."SignatureDefinitions_SignatureDefIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."SignatureDefinitions_SignatureDefIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."SignatureDefinitions_SignatureDefIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."SignatureDefinitions_SignatureDefIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."SignatureDefinitions_SignatureDefIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3391 (class 0 OID 0)
-- Dependencies: 268
-- Name: TABLE "SystemLogs"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."SystemLogs" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."SystemLogs" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."SystemLogs" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."SystemLogs" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."SystemLogs" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."SystemLogs" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."SystemLogs" TO "DbBackupUser";


--
-- TOC entry 3393 (class 0 OID 0)
-- Dependencies: 269
-- Name: SEQUENCE "SystemLogs_EntryIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."SystemLogs_EntryIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."SystemLogs_EntryIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."SystemLogs_EntryIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."SystemLogs_EntryIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."SystemLogs_EntryIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."SystemLogs_EntryIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."SystemLogs_EntryIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3394 (class 0 OID 0)
-- Dependencies: 270
-- Name: TABLE "UserProperties"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."UserProperties" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."UserProperties" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."UserProperties" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."UserProperties" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."UserProperties" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."UserProperties" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."UserProperties" TO "DbBackupUser";


--
-- TOC entry 3396 (class 0 OID 0)
-- Dependencies: 271
-- Name: SEQUENCE "UserProperties_PropertyIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."UserProperties_PropertyIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."UserProperties_PropertyIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."UserProperties_PropertyIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."UserProperties_PropertyIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."UserProperties_PropertyIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."UserProperties_PropertyIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."UserProperties_PropertyIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3397 (class 0 OID 0)
-- Dependencies: 272
-- Name: TABLE "Users"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."Users" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."Users" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."Users" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."Users" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."Users" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."Users" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."Users" TO "DbBackupUser";


--
-- TOC entry 3399 (class 0 OID 0)
-- Dependencies: 273
-- Name: SEQUENCE "Users_UserIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."Users_UserIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Users_UserIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."Users_UserIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."Users_UserIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Users_UserIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Users_UserIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."Users_UserIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3400 (class 0 OID 0)
-- Dependencies: 274
-- Name: TABLE "Workflows"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."Workflows" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."Workflows" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."Workflows" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."Workflows" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."Workflows" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."Workflows" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."Workflows" TO "DbBackupUser";


--
-- TOC entry 3402 (class 0 OID 0)
-- Dependencies: 275
-- Name: SEQUENCE "Workflows_WorkflowIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."Workflows_WorkflowIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Workflows_WorkflowIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."Workflows_WorkflowIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."Workflows_WorkflowIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Workflows_WorkflowIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Workflows_WorkflowIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."Workflows_WorkflowIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 3403 (class 0 OID 0)
-- Dependencies: 276
-- Name: TABLE "Worklists"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON TABLE "ViCellInstrument"."Worklists" FROM "BCIViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."Worklists" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."Worklists" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON TABLE "ViCellInstrument"."Worklists" TO "ViCellAdmin";
GRANT ALL ON TABLE "ViCellInstrument"."Worklists" TO "ViCellDBAdmin";
GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLE "ViCellInstrument"."Worklists" TO "ViCellInstrumentUser";
GRANT SELECT,REFERENCES,TRIGGER ON TABLE "ViCellInstrument"."Worklists" TO "DbBackupUser";


--
-- TOC entry 3405 (class 0 OID 0)
-- Dependencies: 277
-- Name: SEQUENCE "Worklists_WorklistIdNum_seq"; Type: ACL; Schema: ViCellInstrument; Owner: BCIViCellAdmin
--

REVOKE ALL ON SEQUENCE "ViCellInstrument"."Worklists_WorklistIdNum_seq" FROM "BCIViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Worklists_WorklistIdNum_seq" TO "BCIViCellAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."Worklists_WorklistIdNum_seq" TO "BCIDBAdmin" WITH GRANT OPTION;
GRANT ALL ON SEQUENCE "ViCellInstrument"."Worklists_WorklistIdNum_seq" TO "ViCellAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Worklists_WorklistIdNum_seq" TO "ViCellDBAdmin";
GRANT ALL ON SEQUENCE "ViCellInstrument"."Worklists_WorklistIdNum_seq" TO "ViCellInstrumentUser";
GRANT SELECT,USAGE ON SEQUENCE "ViCellInstrument"."Worklists_WorklistIdNum_seq" TO "DbBackupUser";


--
-- TOC entry 1959 (class 826 OID 142753)
-- Name: DEFAULT PRIVILEGES FOR TABLES; Type: DEFAULT ACL; Schema: -; Owner: BCIViCellAdmin
--

ALTER DEFAULT PRIVILEGES FOR ROLE "BCIViCellAdmin" REVOKE ALL ON TABLES  FROM "BCIViCellAdmin";
ALTER DEFAULT PRIVILEGES FOR ROLE "BCIViCellAdmin" GRANT ALL ON TABLES  TO "BCIViCellAdmin" WITH GRANT OPTION;
ALTER DEFAULT PRIVILEGES FOR ROLE "BCIViCellAdmin" GRANT ALL ON TABLES  TO "BCIDBAdmin" WITH GRANT OPTION;
ALTER DEFAULT PRIVILEGES FOR ROLE "BCIViCellAdmin" GRANT ALL ON TABLES  TO "ViCellAdmin";
ALTER DEFAULT PRIVILEGES FOR ROLE "BCIViCellAdmin" GRANT ALL ON TABLES  TO "ViCellDBAdmin";
ALTER DEFAULT PRIVILEGES FOR ROLE "BCIViCellAdmin" GRANT SELECT,INSERT,REFERENCES,TRIGGER,TRUNCATE,UPDATE ON TABLES  TO "ViCellInstrumentUser";
ALTER DEFAULT PRIVILEGES FOR ROLE "BCIViCellAdmin" GRANT SELECT,REFERENCES,TRIGGER ON TABLES  TO "DbBackupUser";


-- Completed on 2020-12-29 11:08:04

--
-- PostgreSQL database dump complete
--


ALTER ROLE postgres WITH SUPERUSER INHERIT CREATEROLE CREATEDB LOGIN REPLICATION BYPASSRLS PASSWORD 'md5c5d2e02306e8cbdd4740165f9f2b9575';

