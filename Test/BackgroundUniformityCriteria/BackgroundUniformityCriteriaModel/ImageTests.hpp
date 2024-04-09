#pragma once

#include "IExecute.hpp"

class TestCompleteImageAvg : public IExecute
{
public:
	TestCompleteImageAvg(const ExecuteInputParameters& inParams, double offset);

	// Inherited via IExecute
	virtual bool execute(std::shared_ptr<cv::Mat> image) override;
	virtual void getResult(ExecuteResult & result) override;
	virtual std::string testName() override;

private:
	ExecuteInputParameters inputParams_;
	double offSet_;
};


class TestImageMean : public IExecute
{
public:
	TestImageMean(const ExecuteInputParameters& inParams);

	// Inherited via IExecute
	virtual bool execute(std::shared_ptr<cv::Mat> image) override;
	virtual void getResult(ExecuteResult & result) override;
	virtual std::string testName() override;

private:
	ExecuteInputParameters inputParams_;
};

class TestCofficientVar : public IExecute
{
public:
	TestCofficientVar(const ExecuteInputParameters& inParams);

	// Inherited via IExecute
	virtual bool execute(std::shared_ptr<cv::Mat> image) override;
	virtual void getResult(ExecuteResult & result) override;
	virtual std::string testName() override;

private:
	ExecuteInputParameters inputParams_;
};